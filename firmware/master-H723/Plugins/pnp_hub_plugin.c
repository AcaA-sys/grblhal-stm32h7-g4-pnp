/**
  ******************************************************************************
  * @file    pnp_hub_plugin.c
  * @author  Проект: grblhal-stm32h7-g4-pnp
  * @version V2.0
  * @date    26-Август-2026
  * @brief   Основной высокоскоростной плагин ядра grblHAL для мастера H723.
  *          Обеспечивает перехват траектории 2кГц, работу SPI2 DMA и логгер T1-T4.
  ******************************************************************************
  */

#include <stdio.h>
#include <string.h>
#include "grbl/hal.h"
#include "grbl/grbl.h"
#include "stm32h7xx_hal.h"
#include "my_machine.h"
#include "master_main.h"

/* Внешние аппаратные хэндлы из main.c */
extern SPI_HandleTypeDef hspi2;
extern CRC_HandleTypeDef hcrc;

/* Буферы обмена, размещенные в Non-Cacheable RAM (SRAM2) для исключения джиттера D-Cache */
__attribute__((section(".non_cacheable_ram"))) __attribute__((aligned(32))) 
AxisPacket48_t spi_tx_packet;

__attribute__((section(".non_cacheable_ram"))) __attribute__((aligned(32))) 
volatile uint32_t spi_rx_raw_timestamps; // Массив из 2-х uint32_t под [T3, T4]

/* Организация кольцевого буфера для асинхронного USB-логгера таймингов */
#define LOG_BUF_SIZE 16
static TelemetryLog_t log_buffer[LOG_BUF_SIZE];
static volatile uint8_t log_w_idx = 0;
static volatile uint8_t log_r_idx = 0;

/* Указатель на цепочку оригинального обработчика grblHAL */
static stepper_pulse_start_ptr on_pulse_start_chained;
static uint32_t global_segment_counter = 0;

/**
  * @brief  Высокоприоритетный хук планировщика траектории ЧПУ (Вызывается строго каждые 0.5 мс)
  * @param  prep Указатель на структуру подготовленного блока движения grblHAL
  * @retval None
  */
static void pnp_spi_pulse_start(st_prep_t *prep)
{
    /* МГНОВЕННЫЙ ЗАХВАТ МЕТКИ T1: Координаты кванта рассчитаны планировщиком ядра */
    uint32_t t1 = DWT->CYCCNT;

    /* Защита от наложения транзакций шины SPI */
    if (hspi2.State == HAL_SPI_STATE_BUSY_TX_RX) {
        return; 
    }

    /* Инкремент сквозного счетчика сегментов */
    if (prep && prep->ev_buffer) {
        global_segment_counter++;
    }
    spi_tx_packet.segment_id = global_segment_counter;

    /* Сбор честных текущих координат 8 осей из ядра grblHAL (XYZUVR1R2) */
    sys_position_t pos;
    hal.get_position(&pos); 
    
    // Передаем координаты транзитом в шагах из регистров планировщика
    for (int i = 0; i < N_AXIS; i++) {
        spi_tx_packet.positions[i] = pos.steps[i];
    }

    /* Наполнение исполнительной ПЛК-периферии головки (Клапаны вакуума, продувки, свет) */
    // Значения считываются из кастомных регистров M-кодов grblHAL
    spi_tx_packet.valves_vacuum  = (uint8_t)(sys.io_port & 0xFF);         // Пример маски
    spi_tx_packet.valves_blow    = (uint8_t)((sys.io_port >> 8) & 0xFF);  // Пример маски
    spi_tx_packet.led_brightness = 255;                                   // Полная яркость по умолчанию

    /* Передача текущего статуса автомата состояний ЧПУ */
    spi_tx_packet.machine_state = (uint16_t)sys.state;
    spi_tx_packet.reserved      = 0xAA; // Маркер выравнивания

    /* Расчет аппаратного CRC16-CCITT всего пакета за исключением поля CRC */
    uint32_t words_to_calc = (sizeof(AxisPacket48_t) - 2) / 4; // Объем в 32-битных словах
    spi_tx_packet.crc16 = (uint16_t)HAL_CRC_Calculate(&hcrc, (uint32_t*)&spi_tx_packet, words_to_calc);

    /* МГНОВЕННЫЙ ЗАХВАТ МЕТКИ T2: Пакет упакован, CRC посчитан, готов к вылету в медь */
    uint32_t t2 = DWT->CYCCNT;

    /* Логирование локальных меток в циклический буфер */
    uint8_t next_w_idx = (log_w_idx + 1) % LOG_BUF_SIZE;
    if (next_w_idx != log_r_idx) {
        log_buffer[log_w_idx].segment_id     = global_segment_counter;
        log_buffer[log_w_idx].t1_generated_h7 = t1;
        log_buffer[log_w_idx].t2_spi_start_h7 = t2;
        
        /* ВАЖНО: В этот момент в массиве spi_rx_raw_timestamps лежат [T3, T4] ПРОШЛОГО такта,
           которые хаб G474 успел передать обратно по линии MISO во время текущей сессии! */
        log_buffer[log_w_idx].t3_spi_end_g4   = spi_rx_raw_timestamps;
        log_buffer[log_w_idx].t4_sync_sent_g4 = spi_rx_raw_timestamps;
        
        log_w_idx = next_w_idx;
    }

    /* ДВУНАПРАВЛЕННЫЙ ВЫСТРЕЛ SPI2 DMA: отправляем уставку, забираем тайминги хаба */
    // Ножка NSS (PB12) падает аппаратно, запуская трансляцию кадра
    HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&spi_tx_packet, (uint8_t*)spi_rx_raw_timestamps, sizeof(AxisPacket48_t));

    /* Вызов оригинального обработчика grblHAL, если он был зарегистрирован ранее */
    if (on_pulse_start_chained) {
        on_pulse_start_chained(prep);
    }
}

/**
  * @brief  Фоновый воркер отправки сквозных наносекундных логов Latency в USB VCP.
  *         Вызывается асинхронно в главном цикле main.c, не внося джиттер в ЧПУ.
  * @retval None
  */
void master_telemetry_usb_worker(void)
{
    char usb_buffer;

    while (log_r_idx != log_w_idx) {
        TelemetryLog_t log = log_buffer[log_r_idx];

        /* Математический пересчет тактов процессора H723 (550 МГц) в микросекунды */
        // 1 микросекунда = ровно 550 тиков счетчика DWT->CYCCNT
        float t_generation = (float)(log.t2_spi_start_h7 - log.t1_generated_h7) / 550.0f;
        float t_spi_transit = (float)(log.t3_spi_end_g4 - log.t2_spi_start_h7) / 550.0f;
        float t_can_sync    = (float)(log.t4_sync_sent_g4 - log.t3_spi_end_g4) / 550.0f;
        float t_total_delay = (float)(log.t4_sync_sent_g4 - log.t1_generated_h7) / 550.0f;

        /* Формирование высокоточного лога для вывода в ioSender / консоль ПК */
        // Используем явное ограничение по точности до 1 знака после запятой
        snprintf(usb_buffer, sizeof(usb_buffer),
                 "SEG:%lu | GEN:%0.1fus | SPI_LTC:%0.1fus | CAN_SYNC:%0.1fus | TOTAL:%0.1fus\r\n",
                 log.segment_id, t_generation, t_spi_transit, t_can_sync, t_total_delay);

        /* Прямой неблокирующий вылет строки в стандартный USB CDC поток grblHAL */
        hal.stream.write(usb_buffer);

        /* Инкремент указателя чтения кольцевого буфера */
        log_r_idx = (log_r_idx + 1) % LOG_BUF_SIZE;
    }
}

/**
  * @brief  Инициализация и бесшовная регистрация плагина в архитектуре grblHAL
  * @retval None
  */
void master_plugin_init(void)
{
    // Очищаем входные/выходные некэшируемые структуры перед стартом
    memset(&spi_tx_packet, 0, sizeof(AxisPacket48_t));
    spi_rx_raw_timestamps = 0;
    spi_rx_raw_timestamps = 0;

    // Перехватываем стандартный вектор начала шагового периода, выстраивая цепочку (Chain)
    on_pulse_start_chained = hal.stepper.pulse_start;
    hal.stepper.pulse_start = pnp_spi_pulse_start;
}
