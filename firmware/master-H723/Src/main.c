/**
  ******************************************************************************
  * @file    main.c
  * @author  Проект: grblhal-stm32h7-g4-pnp
  * @version V2.0
  * @date    26-Август-2026
  * @brief   Главный файл инициализации и фонового диспетчера мастера H723.
  *          Обеспечивает конфигурацию PLL 550МГц, MCO1 25МГц и запуск grblHAL.
  ******************************************************************************
  */

#include "main.h"
#include "my_machine.h"
#include "master_main.h"

/* Хэндлы аппаратной периферии (CubeMX экспорт) */
ADC_HandleTypeDef hadc1;
DAC_HandleTypeDef hdac1;
I2C_HandleTypeDef hi2c1;
SPI_HandleTypeDef hspi2;
UART_HandleTypeDef huart2;
CRC_HandleTypeDef hcrc;

/* Объявление глобальных переменных (согласно master_main.h) */
bool is_counterfeit_board = false;
bool is_counterfeit_bricked = false;
uint8_t active_sabotage_mode = 0;

uint8_t current_jog_axis = 0;
uint8_t current_jog_scale = 0;
bool deadman_button_pressed = false;

/* Прототипы локальных системных функций */
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI2_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_DAC1_Init(void);
static void MX_USART2_UART_Init(void);

/**
  * @brief  Главная точка входа в программу.
  * @retval int
  */
int main(void)
{
    /* 1. Сброс всей периферии, инициализация интерфейса Flash и системного тика Flash */
    HAL_Init();

    /* 2. Конфигурация тактового поля: Разгон ядра до 550 МГц, выдача 25 МГц на PA8 (MCO1) */
    SystemClock_Config();

    /* 3. Системный запуск аппаратного наносекундного счетчика циклов */
    master_dwt_init();

    /* 4. Инициализация базовой периферии и доменов DMA */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_CRC_Init();
    MX_SPI2_Init();
    MX_I2C1_Init();
    MX_ADC1_Init();
    MX_DAC1_Init();
    MX_USART2_UART_Init();

    /* 5. Инициализация подсистемы антиконтрафакта */
    // Опрашивает внешнюю EEPROM 24LC16B, проверяет флаги и декрементирует счетчик запусков
    sec_crypto_init();
    sec_update_startup_counter();

    /* 6. Регистрация и запуск плагина ЧПУ (хук на pulse_start 2 кГц) */
    master_plugin_init();

    /* 7. Инициализация и запуск основного ядра grblHAL ЧПУ */
    // Эта функция инициализирует парсер G-кода, USB-стек ioSender и забирает управление прерываниями
    grbl_init(); 

    /* Финальный программный барьер — если хаб опустил READY (PD3), grblHAL уйдет в HARD_ALARM */
    if (HAL_GPIO_ReadPin(READY_PORT, READY_PIN) == GPIO_PIN_RESET) {
        // Сигнализируем ядру ЧПУ, что ведомое устройство не готово к работе
        sys.state = STATE_ALARM; 
    }

    /* 8. ГЛАВНЫЙ БЕСКОНЕЧНЫЙ ЦИКЛ ОБРАБОТКИ ФОНОВЫХ ЗАДАЧ */
    while (1)
    {
        /* Задача А: Обслуживание ядра grblHAL (Обработка потока команд USB от OpenPnP) */
        grbl_run();

        /* Задача Б: Скрытая периодическая проверка нелинейного RLC контура защиты B1 */
        // Вызывается асинхронно раз в 40 стартов, когда станок простаивает в Idle
        if (sys.state == STATE_IDLE) {
            sec_verify_hardware_loop();
        }

        /* Задача В: Программный опрос и дебаунс аналоговых переключателей пульта MPG */
        // Считывает АЦП на PA4/PA5, высчитывает активную ось и шаг
        mpg_process_analog_switches();

        /* Задача Г: Выплевывание сквозных наносекундных логов T1->T4 в USB VCP */
        // Работает из кольцевого буфера, полностью разгружая прерывания ЧПУ
        master_telemetry_usb_worker();
    }
}

/**
  * @brief  Жесткая конфигурация тактирования (PLL 550 МГц + HSE Bypass вывод 25 МГц)
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Настройка основного ФАПЧ (PLL1) от внешнего кварца 25 МГц */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;     /* 25 / 5 = 5 МГц */
    RCC_OscInitStruct.PLL.PLLN = 220;   /* 5 * 220 = 1100 МГц */
    RCC_OscInitStruct.PLL.PLLP = 2;     /* 1100 / 2 = 550 МГц (SYSCLK) */
    RCC_OscInitStruct.PLL.PLLQ = 2;
    RCC_OscInitStruct.PLL.PLLR = 2;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* Конфигурация делителей внутренних шин Core, AHB, APB */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK

                                 |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                                 |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;       /* AHB = 275 МГц */
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;     /* APB3 = 137.5 МГц */
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;     /* APB1 = 137.5 МГц */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;     /* APB2 = 137.5 МГц */
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;     /* APB4 = 137.5 МГц */
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3);

    /* АППАРАТНЫЙ ВЫВОД ТАКТОВОЙ ЧАСТОТЫ НА ПИН PA8 (MCO1) ДЛЯ ХАБА G474 */
    // Выдаем сырую частоту 25 МГц кварца напрямую без деления (DIV_1)
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
}

/**
  * @brief  Инициализация GPIO общего назначения (READY, LED, EEPROM_WP)
  * @retval None
  */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();

    /* 1. Настройка входного пина READY (PD3) как EXTI Прерывания по спаду */
    GPIO_InitStruct.Pin = READY_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING; // Срабатывает мгновенно при падении в 0
    GPIO_InitStruct.Pull = GPIO_PULLUP;          // Безопасный Pull-Up, дублирующий внешний Pull-Down
    HAL_GPIO_Init(READY_PORT, &GPIO_InitStruct);

    /* Аппаратная привязка EXTI3 к NVIC и выставление наивысшего приоритета защиты */
    HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0); 
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);

    /* 2. Настройка пина аппаратной защиты записи EEPROM WP (PB5) */
    GPIO_InitStruct.Pin = EEPROM_WP_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EEPROM_WP_PORT, &GPIO_InitStruct);
    // По умолчанию держим HIGH (Память заперта для записи)
    HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
}

/**
  * @brief  Инициализация аппаратного блока аппаратного расчета CRC16
  * @retval None
  */
static void MX_CRC_Init(void)
{
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_DISABLE;
    hcrc.Init.GeneratingPolynomial = 0x1021; // Полином CRC16-CCITT
    hcrc.Init.CRCLength = CRC_POLYLENGTH_16B;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_WORDS;
    HAL_CRC_Init(&hcrc);
}

/**
  * @brief  Инициализация межпроцессорной шины SPI2 (Slave Mode)
  * @retval None
  */
static void MX_SPI2_Init(void)
{
    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_SLAVE;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_HARD_INPUT; // Аппаратный строб кадра на PB12
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    HAL_SPI_Init(&hspi2);
}

/**
  * @brief  Инициализация шины I2C1 для системной памяти 24LC16B
  * @retval None
  */
static void MX_I2C1_Init(void)
{
    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = 0x00C03F5D; // Скорость 400 кГц (Fast Mode) при тактовой шины
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressesMode = I2C_ADDRESSESMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    HAL_I2C_Init(&hi2c1);
}

/**
  * @brief  Инициализация быстрых каналов АЦП (PA4, PA5, PA6)
  * @retval None
  */
static void MX_ADC1_Init(void)
{
    ADC_MultiModeTypeDef multimode = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B; // 12-битное разрешение (0-4095)
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    HAL_ADC_Init(&hadc1);

    multimode.Mode = ADC_MODE_INDEPENDENT;
    HAL_ADCEx_MultiModeConfigChannel(&hadc1, &multimode);
}

/**
  * @brief  Инициализация ЦАП секретки защиты (Пин PA4)
  * @retval None
  */
static void MX_DAC1_Init(void)
{
    DAC_ChannelConfTypeDef sConfig = {0};

    hdac1.Instance = DAC1;
    HAL_DAC_Init(&hdac1);

    sConfig.DAC_Trigger = DAC_TRIGGER_NONE; // Прямое управление выходом из кода
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE;
    HAL_DAC_ConfigChannel(&hdac1, &sConfig, DAC_CHANNEL_1);
}

/**
  * @brief  Инициализация изолированной шины фидеров RS-485 (USART2)
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = RS485_BAUDRATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1B;
