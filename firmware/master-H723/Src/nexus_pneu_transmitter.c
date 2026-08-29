/**
  ******************************************************************************
  * @file    nexus_pneu_transmitter.c
  * @brief   Пневматический транслятор опорных давлений станины для grblHAL
  * @part    STM32H723 (Материнская плата Nexus-H7-FD)
  * @note    Оцифровывает датчики XGZP6847 (делители 10к/20к) и выстреливает 
  *          бинарный пакет в оптический ИК-мост связи CAN FD2 (5 Мбит/с).
  ******************************************************************************
  */

#include "grbl.h" // Подключение заголовков ядра grblHAL
#include "main.h"

/* Внешние аппаратные хэндлы, генерируемые STM32CubeMX на мастере H723 */
extern ADC_HandleTypeDef hadc1;   // Монопольное АЦП1 для датчиков магистралей
extern FDCAN_HandleTypeDef hfdcan2; // Оптическая изолированная шина ИК-луча (5 Мбит/с)

/* Физическая привязка пинов датчиков XGZP6847 на мастере */
#define VACUUM1_PIN                GPIO_PIN_2  // PA2 (ADC1_IN14) / Вход давления 0.4 Бар
#define VACUUM2_PIN                GPIO_PIN_3  // PA3 (ADC1_IN15) / Вход вакуума -1 Бар
#define CAN_ID_ENV_DATA            0x410       // ID кадра экологических и пневмо-данных

/* Глобальные буферы хранения сырых 12-битных кодов АЦП мастера */
volatile uint16_t master_p1_pressure_raw = 0;
volatile uint16_t master_p2_vacuum_raw   = 0;

/**
  * @brief  Аппаратный запуск регулярного сэмплирования датчиков давления XGZP6847
  * @note   Вызывается один раз при инициализации плагинов в main() мастера H723.
  */
void Nexus_Master_Pneumatics_Init(void)
{
    // Настройка пинов PA2 и PA3 на мастере H723 в аналоговый режим выполняется в main.c
    // Запускаем АЦП1 в режиме сканирования каналов (Scan Mode) или через прерывания/DMA
    HAL_ADC_Start(&hadc1); 
}

/**
  * @brief  Опрос аналоговых каналов и обновление переменных давления мастера
  * @note   Вызывается регулярно в фоновом цикле ЧПУ или по таймеру кванта (0.5 мс)
  */
void Nexus_Master_Read_Pneumatic_Sensors(void)
{
    // Опрашиваем первый регулярный канал (PA2 - Давление 0.4 Бар)
    if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK)
    {
        master_p1_pressure_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    
    // Опрашиваем второй регулярный канал (PA3 - Вакуум -1 Бар)
    if (HAL_ADC_PollForConversion(&hadc1, 1) == HAL_OK)
    {
        master_p2_vacuum_raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
}

/**
  * @brief  ВЫСТРЕЛ ДАННЫХ В ОПТИЧЕСКИЙ ИК-МОСТ: Отправка пневматики по шине CAN FD2
  * @note   Вызывается строго в моменты остановок ЧПУ (команда M400 / состояние IDLE)
  *         или принудительно перед началом смены инструмента / укладки детали.
  */
void Nexus_Master_Transmit_Pneu_To_Head(float head_current_temperature)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    uint8_t TxData[8];
    
    // 1. Считываем свежие физические показания датчиков XGZP6847 мастера
    Nexus_Master_Read_Pneumatic_Sensors();
    
    // 2. Упаковываем текущую температуру головы (float), прилетевшую ранее, в байты 0-3 кадра
    // (Если температура не используется мастером, байты забиваются нулями или локальным флудом)
    memcpy(&TxData[0], &head_current_temperature, sizeof(float));
    
    // 3. Пакуем 12-битные сырые коды АЦП общего давления 0.4 Бар (PA2) в байты 4-5
    TxData[4] = (uint8_t)(master_p1_pressure_raw & 0xFF);
    TxData[5] = (uint8_t)((master_p1_pressure_raw >> 8) & 0xFF);
    
    // 4. Пакуем 12-битные сырые коды АЦП центрального вакуума -1 Бар (PA3) в байты 6-7
    TxData[6] = (uint8_t)(master_p2_vacuum_raw & 0xFF);
    TxData[7] = (uint8_t)((master_p2_vacuum_raw >> 8) & 0xFF);
    
    // 5. Конфигурация заголовка CAN FD фрейма под стандарт N-Bus v1.3
    TxHeader.Identifier = CAN_ID_ENV_DATA; // 0x410
    TxHeader.IdType = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType = FDCAN_DATA_FRAME;
    TxHeader.DataLength = FDCAN_DLC_BYTES_8;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch = FDCAN_BRS_ON; // Аппаратный разгон оптического ИК-луча до 5 Мбит/с!
    TxHeader.FDFormat = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker = 0;
    
    // 6. Выстрел пакета в оптический домен CAN FD2. 
    // Данные летят по воздуху прямо в модуль CSNP1GCR01-AOW на голове.
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, TxData);
}
