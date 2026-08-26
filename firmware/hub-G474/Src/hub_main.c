/**
  ******************************************************************************
  * @file    hub_main.c
  * @author  Проект: grblhal-stm32h7-g4-pnp
  * @version V2.0
  * @date    26-Август-2026
  * @brief   Основной рабочий модуль маршрутизатора хаба STM32G474.
  *          Обеспечивает перехват T3/T4, сквозной раскид CAN FD и выстрел SYNC.
  ******************************************************************************
  */

#include "main.h"
#include "hub_config.h"
#include <string.h>

/* Экспорт аппаратных хэндлов периферии (CubeMX) */
extern SPI_HandleTypeDef  hspi3;   // Slave Mode, Hardware NSS Enabled (PB12)
extern CRC_HandleTypeDef  hcrc;    // Аппаратный блок CRC16-CCITT (Полином 0x1021)
extern FDCAN_HandleTypeDef hfdcan1; // Магистраль Портала (X/Y BLDC на медь)
extern FDCAN_HandleTypeDef hfdcan2; // Магистраль Головки (Z/R и датчики на оптику)

/* Буферы обмена, выровненные по границам слов памяти */
__attribute__((aligned(32))) CNC_Packet_t rx_packet;
__attribute__((aligned(32))) HubTelemetry_t tx_telemetry_back;

/* Временные метки хаба (Шкала тактов ядра 170 МГц) */
volatile uint32_t t3_nss_fallback_ticks = 0;
volatile uint32_t t4_can_sync_sent_ticks = 0;

/* Глобальные структуры заголовков сообщений CAN FD */
static FDCAN_TxHeaderTypeDef TxHeader_Axis;
static FDCAN_TxHeaderTypeDef TxHeader_Sync;

/* Буферы Message RAM для мгновенного копирования данных */
static uint8_t can_axis_payload[8];

/**
  * @brief  Первичная аппаратная инициализация и запуск контуров связи хаба.
  * @retval None
  */
void hub_init(void)
{
    // 1. По умолчанию при старте держим аварийную линию READY в состоянии LOW (Блокируем мастер)
    HAL_GPIO_WritePin(READY_PORT, READY_PIN, GPIO_PIN_RESET);

    // Читаем DIP-переключатели или Solder-перемычки железного конфигуратора
    hub_read_hardware_config();

    // 2. Предварительная конфигурация заголовка для информационных кадров осей
    TxHeader_Axis.IdType = FDCAN_STANDARD_ID;
    TxHeader_Axis.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader_Axis.DataLength = FDCAN_DLC_BYTES_8;    // Каждая ось пакуется в свой 8-байтный кадр
    TxHeader_Axis.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader_Axis.BitRateSwitch = FDCAN_BRS_ON;      // Включаем разгон Data Phase до 4 Мбит/с
    TxHeader_Axis.FDFormat = FDCAN_FD_CAN;
    TxHeader_Axis.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader_Axis.MessageMarker = 0;

    // 3. Предварительная конфигурация пускового кадра ЖЕСТКОЙ СИНХРОНИЗАЦИИ
    TxHeader_Sync.Identifier = CAN_ID_HARD_SYNC;     // ID 0x000 (Высший приоритет шины)
    TxHeader_Sync.IdType = FDCAN_STANDARD_ID;
    TxHeader_Sync.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader_Sync.DataLength = FDCAN_DLC_BYTES_0;     // Пустой маркер, 0 байт полезных данных
    TxHeader_Sync.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader_Sync.BitRateSwitch = FDCAN_BRS_ON;
    TxHeader_Sync.FDFormat = FDCAN_FD_CAN;
    // Настраиваем запись события отправки в FIFO — строго необходимо для фиксации T4!
    TxHeader_Sync.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
    TxHeader_Sync.MessageMarker = CAN_SYNC_MARKER;   // Маркер 0xAA для распознавания в прерывании

    // Небольшая аппаратная задержка на релаксацию ИОН REF3033 стабилизации CAN шин
    HAL_Delay(5);

    // Запускаем наносекундный счетчик DWT хаба (170 МГц)
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTMsk;

    // Зажигаем белый светодиод оригинальности платы (PA3 через инверсный буфер)
    HAL_GPIO_WritePin(LED_PORT_C, LED_PIN_SECRET_STATUS, GPIO_PIN_RESET);

    // Поднимаем линию READY (PB2) в состояние HIGH — даем мастеру H723 зеленый свет на старт grblHAL
    HAL_GPIO_WritePin(READY_PORT, READY_PIN, GPIO_PIN_SET);

    /* ЗАПУСК ДВУНАПРАВЛЕННОГО КОЛЬЦЕВОГО ОБМЕНА SPI3 DMA SLAVE */
    // Хаб встает в бесконечный цикл ожидания падения NSS от мастера
    HAL_SPI_TransmitReceive_DMA(&hspi2, (uint8_t*)&tx_telemetry_back, (uint8_t*)&rx_packet, sizeof(CNC_Packet_t));
}

/**
  * @brief  МГНОВЕННЫЙ ПЕРЕХВАТ МЕТКИ T3: Внешнее аппаратное прерывание по спаду NSS.
  *         Пин PB12 физически соединен на плате со входом EXTI15_10_IRQn.
  * @param  GPIO_Pin Номер сработавшего вывода
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_12) // Замените на реальный индекс пина EXTI вашей разводки
    {
        /* МЕТКА T3: Поймана первая наносекунда начала транзакции SPI моста */
        t3_nss_fallback_ticks = DWT->CYCCNT;
    }
}

/**
  * @brief  ВЫСОКОПРИОРИТЕТНЫЙ СКВОЗНОЙ МАРШРУТИЗАТОР.
  *         Вызывается аппаратно по окончании приема 48 байт пакета по SPI2 DMA.
  * @param  hspi Указатель на хэндл SPI
  * @retval None
  */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2)
    {
        /* 1. Аппаратная проверка контрольной суммы прилетевшего каванта траектории */
        uint32_t words_to_calc = (sizeof(CNC_Packet_t) - 2) / 4;
        uint16_t calculated_crc = (uint16_t)HAL_CRC_Calculate(&hcrc, (uint32_t*)&rx_packet, words_to_calc);

        if (calculated_crc != rx_packet.crc16)
        {
            // Пакет битый (помеха на треке SPI). Пропускаем такт, моторы идут по экстраполяции.
            return;
        }

        /* 2. Подготовка телеметрии для следующего такта MISO */
        // Пересчитываем такты хаба (170 МГц) в шкалу мастера (550 МГц) для линейности лога на ПК
        // Масштабный коэффициент = 550.0 / 170.0 = 3.235294
        tx_telemetry_back.t3_spi_end_g4   = (uint32_t)((double)t3_nss_fallback_ticks * 3.235294);
        tx_telemetry_back.t4_sync_sent_g4 = (uint32_t)((double)t4_can_sync_sent_ticks * 3.235294);

        /* МАКРОС САБОТАЖА АНТИКЛОНА: Проверка команды скрытого дропа пакетов */
        if (rx_packet.machine_state == 0xEEEE)
        {
            // Мастер приказал симулировать обрыв шлейфа головы. Пропускаем FDCAN2 оптики головки.
            goto skip_fdcan2_head;
        }

        // ==============================================================================
        // КАНАЛ 1: FDCAN1 — Магистраль Портала (BLDC моторы X и Y на меди)
        // ==============================================================================
        // Вытаскиваем координаты X (positions[0]) и Y (positions[1])
        for (uint32_t i = 0; i < 2; i++)
        {
            TxHeader_Axis.Identifier = CAN_ID_PORTAL_BASE + i; // ID: 0x101 (X), 0x102 (Y)
            memcpy(can_axis_payload, &rx_packet.positions[i], 4);
            
            // Заливаем дополнительные ПЛК-флаги в оставшиеся 4 байта кадра при необходимости
            memset(&can_axis_payload[4], 0, 4);

            if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan1) > 0) {
                HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader_Axis, can_axis_payload);
            }
        }

        // ==============================================================================
        // КАНАЛ 2: FDCAN2 — Магистраль Умной Головки (Оси Z1..Z4, R1..R2 на Оптике)
        // ==============================================================================
        // Вытаскиваем оставшиеся 6 честных осей планировщика (Индексы 2..7)
        for (uint32_t i = 2; i < N_AXIS; i++)
        {
            TxHeader_Axis.Identifier = CAN_ID_HEAD_BASE + (i - 2); // ID: 0x201..0x206
            memcpy(can_axis_payload, &rx_packet.positions[i], 4);
            
            // В хвост кадра оси Z1 тайно подмешиваем маски вакуумных клапанов и ШИМ света головки
            if (i == 2) {
                can_axis_payload[4] = rx_packet.valves_vacuum;
                can_axis_payload[5] = rx_packet.valves_blow;
                can_axis_payload[6] = rx_packet.led_brightness;
                can_axis_payload[7] = (uint8_t)rx_packet.segment_id;
            } else {
                memset(&can_axis_payload[4], 0, 4);
            }

            if (HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan2) > 0) {
                HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader_Axis, can_axis_payload);
            }
        }

        // ==============================================================================
        // ЗАЛПОВЫЙ ВЫСТРЕЛ АППАРТНОГО SYNC ПАКЕТА (ID 0x000)
        // ==============================================================================
        skip_fdcan2_head:
        // Все инфо-кадры уже лежат в Message RAM контроллеров CAN. Выдаем пусковой импульс.
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader_Sync, NULL);
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader_Sync, NULL);
    }
}

/**
  * @brief  МГНОВЕННЫЙ ПЕРЕХВАТ МЕТКИ T4: Аппаратное прерывание успешного ухода SYNC кадра.
  *         Вызывается кремнием CAN контроллера, когда пакет физически покинул кристалл.
  * @param  hfdcan Указатель на структуру FDCAN шины
  * @param  TxEventFifoITs Флаги прерываний FIFO
  * @retval None
  */
void HAL_FDCAN_TxEventFifoCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t TxEventFifoITs)
{
    FDCAN_TxEventFifoElement TxEventElem;

    // Достаем элемент события отправки из аппаратной очереди Message RAM
    HAL_FDCAN_GetTxEvent(hfdcan, &TxEventElem);

    // Проверяем, что шину покинул именно наш пусковой маркер жесткой синхронизации
    if (TxEventElem.Identifier == CAN_ID_HARD_SYNC && TxEventElem.MessageMarker == CAN_SYNC_MARKER)
    {
        /* МЕТКА T4: Пакет полетел по меди и оптоволокну к драйверам Drive-FD! */
        t4_can_sync_sent_ticks = DWT->CYCCNT;
    }
}

/**
  * @brief  Считывание конфигурационных перемычек 4-х битного железного конфигуратора.
  * @retval None
  */
void hub_read_hardware_config(void)
{
    uint8_t hardware_bit_mask = 0;

    // Читаем физические уровни пинов (0 - замкнуто на GND, 1 - подтянуто к 3.3В)
    if (HAL_GPIO_ReadPin(CONFIG_PORT_B, CONFIG_PIN_BIT0) == GPIO_PIN_SET) hardware_bit_mask |= (1 << 0);
    if (HAL_GPIO_ReadPin(CONFIG_PORT_B, CONFIG_PIN_BIT1) == GPIO_PIN_SET) hardware_bit_mask |= (1 << 1);
if (HAL_GPIO_ReadPin(CONFIG_PORT_B, CONFIG_PIN_BIT2) == GPIO_PIN_SET) hardware_bit_mask |= (1 << 2);
if (HAL_GPIO_ReadPin(CONFIG_PORT_C, CONFIG_PIN_BIT3) == GPIO_PIN_SET) hardware_bit_mask |= (1 << 3);
// Здесь прошивка может модифицировать тайминги или активировать расширенные режимы
// в зависимости от считанного кода ревизии печатной платы
(void)hardware_bit_mask;
}
/**
•	@brief Контур полной аварийной блокировки хаба (Мгновенный E-Stop станка).
•	@param msg Текстовая причина останова
•	@retval None
/
void hub_emergency_shutdown(const char msg)
{
// За 1 системный такт опускаем READY (PA8) в ноль — мастер H7 уходит в жесткий EXTI-Alarm
HAL_GPIO_WritePin(READY_PORT, READY_PIN, GPIO_PIN_RESET);// Принудительно выключаем белый светодиод оригинальности SECRET_STATUS
HAL_GPIO_WritePin(LED_PORT_C, LED_PIN_SECRET_STATUS, GPIO_PIN_SET);// Аппаратно обесточиваем трансиверы, вгоняя модули FDCAN в режим полного останова
HAL_FDCAN_Stop(&hfdcan1);
HAL_FDCAN_Stop(&hfdcan2);// Уходим в вечный глухой цикл до сброса питания оператором
while (1)
{
__NOP();
}
}

