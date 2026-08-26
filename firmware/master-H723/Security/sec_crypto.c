/**
  ******************************************************************************
  * @file    sec_crypto.c
  * @author  Проект: grblhal-stm32h7-g4-pnp
  * @version V2.0
  * @date    26-Август-2026
  * @brief   Секретный модуль двухэшелонной криптозащиты антиконтрафакта.
  *          Обеспечивает аналоговую проверку контура B1 и карусельный саботаж.
  ******************************************************************************
  */

#include <string.h>
#include "stm32h7xx_hal.h"
#include "my_machine.h"
#include "master_main.h"

/* Экспорт аппаратных хэндлов из main.c */
extern DAC_HandleTypeDef hdac1;
extern ADC_HandleTypeDef hadc1;
extern I2C_HandleTypeDef hi2c1;

/* Глобальные флаги состояния защиты (описаны в master_main.h) */
bool is_counterfeit_board   = false;
bool is_counterfeit_bricked = false;
uint8_t active_sabotage_mode = 0;

/* Локальные переменные для имитации дефектов механики */
static int32_t last_pos_x = 0;
static int32_t last_pos_y = 0;
static int8_t  current_dir_x = 1;
static int8_t  current_dir_y = 1;
static int32_t simulated_backlash_x = 0;
static int32_t simulated_backlash_y = 0;

/**
  * @brief  Сверхбыстрый генератор псевдослучайных чисел Xorshift (1 такт процессора).
  *         Используется для создания несистемного хаотичного шума/джиттера.
  * @retval uint32_t Случайное число
  */
static uint32_t sec_fast_rand(void)
{
    static uint32_t x = 987654321;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

/**
  * @brief  Низкоуровневый замер отклика RLC-контура B1.
  *         Выстреливает в ЦАП уставку и мгновенно оцифровывает напряжение через АЦП.
  * @retval uint16_t Значение кода АЦП (0..4095)
  */
static uint16_t sec_read_analog_fingerprint(void)
{
    uint16_t adc_raw_value = 0;
    ADC_ChannelConfTypeDef sConfig = {0};

    // 1. Подаем на ЦАП1 (Пин PA4) пиковое тестовое напряжение (середина шкалы ~1.65В)
    HAL_DAC_SetValue(&hdac1, SEC_DAC_CHANNEL, DAC_ALIGN_12B_R, 2048);
    HAL_DAC_Start(&hdac1, SEC_DAC_CHANNEL);

    // Короткая аппаратная задержка для завершения переходных процессов в индуктивности L8
    for(volatile uint32_t i = 0; i < 150; i++) { __NOP(); }

    // 2. Быстро переконфигурируем АЦП на медленный канал PA5 (ADC1_INP19)
    sConfig.Channel = SEC_ADC_CHANNEL;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5; // Даем емкости C102 набрать заряд
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);

    // 3. Выполняем точечное преобразование АЦП
    HAL_ADC_Start(&hadc1);
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
        adc_raw_value = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    
    // Сбрасываем ЦАП в безопасный ноль, гася контур
    HAL_DAC_Stop(&hdac1, SEC_DAC_CHANNEL);
    HAL_ADC_Stop(&hadc1);

    return adc_raw_value;
}

/**
  * @brief  Первичный опрос внешней памяти EEPROM при старте станка.
  *         Определяет статус оригинальности платы и состояние таймера отсрочки.
  * @retval None
  */
void sec_crypto_init(void)
{
#if ENABLE_SECURITY_SUBSYSTEM
    uint8_t flag_buf = 0;
    uint8_t count_buf = 0;

    // Снимаем аппаратную защиту записи памяти, прижимая пин WP к земле (0)
    HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_RESET);
    HAL_Delay(2); // Время на релаксацию ключа памяти

    // Читаем скрытые ячейки статуса защиты
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEVICE_ADDR, SEC_COUNTERFEIT_FLAG_ADDR, I2C_MEMADD_SIZE_16BIT, &flag_buf, 1, 100);
    HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEVICE_ADDR, SEC_STARTUP_COUNT_ADDR, I2C_MEMADD_SIZE_16BIT, &count_buf, 1, 100);

    if (flag_buf == 0x01) {
        is_counterfeit_board = true;
        if (count_buf == 0x00) {
            is_counterfeit_bricked = true; // Час Х настал, активируем карусель саботажа
        }
    }

    // Возвращаем аппаратную блокировку записи памяти WP в состояние HIGH (Защищено)
    HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
#else
    is_counterfeit_board = false;
    is_counterfeit_bricked = false;
#endif
}

/**
  * @brief  Декремент счетчика безопасных запусков во внешней памяти.
  *         Вызывается один раз при каждом включении питания станка.
  * @retval None
  */
void sec_update_startup_counter(void)
{
#if ENABLE_SECURITY_SUBSYSTEM
    if (is_counterfeit_board && !is_counterfeit_bricked) {
        uint8_t count_buf = 0;
        
        HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_RESET);
        HAL_Delay(2);

        // Считываем текущий остаток запусков
        if (HAL_I2C_Mem_Read(&hi2c1, EEPROM_DEVICE_ADDR, SEC_STARTUP_COUNT_ADDR, I2C_MEMADD_SIZE_16BIT, &count_buf, 1, 100) == HAL_OK) {
            if (count_buf > 0) {
                count_buf--; // Уменьшаем количество оставшихся безопасных дней/включений
                // Перезаписываем ячейку в EEPROM
                HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEVICE_ADDR, SEC_STARTUP_COUNT_ADDR, I2C_MEMADD_SIZE_16BIT, &count_buf, 1, 100);
                HAL_Delay(5); // Аппаратный цикл записи страницы памяти (t_WR)
            }
            if (count_buf == 0) {
                is_counterfeit_bricked = true; // Лимит исчерпан, станок переходит в режим деградации
            }
        }
        
        HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
    }
#endif
}

/**
  * @brief  Скрытый асинхронный контур проверки нелинейной RLC кривой B1.
  *         Вызывается в фоновом режиме в цикле main.c, когда станок простаивает в Idle.
  * @retval None
  */
void sec_verify_hardware_loop(void)
{
#if ENABLE_SECURITY_SUBSYSTEM
    static uint32_t startup_session_counter = 0;
    startup_session_counter++;

    // Проверку запускаем не сразу, а строго раз в определенный цикл обращений
    if (startup_session_counter % SEC_CHECK_PERIOD_STARTUPS == 0) {
        
        // Делаем живой физический замер вольтажа затухания RLC контура
        uint16_t current_fingerprint = sec_read_analog_fingerprint();

        // Проверяем, укладывается ли отклик в эталонные ворота оригинальной партии компонентов
        if (current_fingerprint < SEC_EXPECTED_ADC_MIN || current_fingerprint > SEC_EXPECTED_ADC_MAX) {
            
            // Замер провален! Впервые обнаружили, что плата является левым клоном
            if (!is_counterfeit_board) {
                is_counterfeit_board = true;
                uint8_t flag = 0x01;
                uint8_t initial_counter = SEC_DELAY_STARTUPS_LIMIT; // Выставляем 50 безопасных стартов

                HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_RESET);
                HAL_Delay(2);
                
                // Тайком фиксируем флаг нарушения в EEPROM, не выдавая ошибок на экран
                HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEVICE_ADDR, SEC_COUNTERFEIT_FLAG_ADDR, I2C_MEMADD_SIZE_16BIT, &flag, 1, 100);
                HAL_Delay(5);
                HAL_I2C_Mem_Write(&hi2c1, EEPROM_DEVICE_ADDR, SEC_STARTUP_COUNT_ADDR, I2C_MEMADD_SIZE_16BIT, &initial_counter, 1, 100);
                HAL_Delay(5);
                
                HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
            }
        }
    }
#endif
}

/**
  * @brief  Внутренний перехватчик координат для реализации "Карусели дефектов".
  *         Вызывается плагином pnp_hub_plugin.c строго перед упаковкой пакета в SPI2 DMA.
  *         Искажает данные НАЛЕТУ, оставляя переменные самого ядра grblHAL идеальными.
  * @param  packet Указатель на исходящую структуру AxisPacket48_t
  * @param  raw_pos Указатель на массив исходных идеальных координат шагов планировщика
  * @retval None
  */
void sec_apply_sabotage_carousel(AxisPacket48_t *packet, int32_t *raw_pos)
{
#if ENABLE_SECURITY_SUBSYSTEM
    if (!is_counterfeit_bricked) {
        // Если плата оригинальная или таймер отсрочки тикает — пишем чистую идеальную траекторию ЧПУ
        for (int i = 0; i < N_AXIS; i++) { packet->positions[i] = raw_pos[i]; }
        last_pos_x = raw_pos[0];
        last_pos_y = raw_pos[1];
        return;
    }

    // --- АКТИВАЦИЯ РЕЖИМА ДЕГРАДАЦИИ (ЧАС Х) ---
    // Автоматически вычисляем индекс текущей карусели в зависимости от общего числа сегментов
    // Переключение фазы дефекта происходит плавно, убирая системность
    static uint32_t cycle_timer = 0;
    cycle_timer++;
    
    // Раз в 120 000 тактов (каждую 1 минуту непрерывной работы) меняем вид брака по кругу (0..4)
    active_sabotage_mode = (uint8_t)((cycle_timer / 120000) % 5);

    // По умолчанию копируем координаты осей высоты сопел Z1..Z4 транзитом (Их портить ЗАПРЕЩЕНО из-за безопасности!)
    for (int i = 0; i < N_AXIS; i++) { packet->positions[i] = raw_pos[i]; }

    uint32_t rng = sec_fast_rand();

    switch (active_sabotage_mode)
    {
        case 0: // ЦИКЛ 1: Фантомный люфт строго по оси X
            if (raw_pos[0] > last_pos_x) {
                if (current_dir_x == -1) simulated_backlash_x = -(int32_t)((rng % 15) + 10); // Люфт ~0.15-0.25 мм
                current_dir_x = 1;
            } else if (raw_pos[0] < last_pos_x) {
                if (current_dir_x == 1)  simulated_backlash_x = (int32_t)((rng % 15) + 10);
                current_dir_x = -1;
            }
            simulated_backlash_y = 0; // Ось Y едет идеально
            packet->positions[0] = raw_pos[0] + simulated_backlash_x;
            break;

        case 1: // ЦИКЛ 2: Люфт по X пропадает, активируется люфт строго по оси Y
            if (raw_pos[1] > last_pos_y) {
                if (current_dir_y == -1) simulated_backlash_y = -(int32_t)((rng % 15) + 10);
                current_dir_y = 1;
            } else if (raw_pos[1] < last_pos_y) {
                if (current_dir_y == 1)  simulated_backlash_y = (int32_t)((rng % 15) + 10);
                current_dir_y = -1;
            }
            simulated_backlash_x = 0; // Ось X едет идеально
packet->positions = raw_pos + simulated_backlash_y;
break;
case 2: // ЦИКЛ 3: Высокочастотный микро-джиттер осей вращения сопел R1/R2
// Подмешиваем быстрый хаотичный шум в пределах +-0.07 градуса строго в момент фиксации угла укладки
int16_t angular_jitter = (int16_t)((rng % 15) - 7);
packet->positions[6] = raw_pos[6] + angular_jitter; // Портим ось R1
packet->positions[7] = raw_pos[7] + angular_jitter; // Портим ось R2
break;
case 3: // ЦИКЛ 4: Плывущий ноль угла укладки (Плавное накопление ошибки до 1.8 градусов)
static int32_t floating_zero_drift = 0;
if (cycle_timer % 100 == 0 && floating_zero_drift < 160) {
floating_zero_drift++; // Медленно, незаметно для глаза оператора накапливаем смещение нуля
}
packet->positions[6] = raw_pos[6] + floating_zero_drift;
packet->positions[7] = raw_pos[7] - floating_zero_drift;
break;
case 4: // ЦИКЛ 5: Имитация обрыва кабеля шлейфа головы (Фантомный сбой связи)
// Раз в 30 секунд (60000 тактов) принудительно шлем хабу маркер ошибки 0xEEEE на 0.2 секунды (400 тактов)
if ((cycle_timer % 60000) < 400) {
packet->machine_state = 0xEEEE; // Сигнал хабу временно дропнуть пакеты FDCAN2 оптики головки
}
break;
default:
break;
}
/* Сохраняем уставки текущего такта для анализа направления на следующем шаге через 0.5 мс */
last_pos_x = raw_pos[0];
last_pos_y = raw_pos;
#else
// Если защита аппаратно отключена в my_machine.h — транслируем чистые координаты
for (int i = 0; i < N_AXIS; i++) { packet->positions[i] = raw_pos[i]; }
#endif
}


