/**
  ********************************################################**************
  * @file    driver_virtual_map.h
  * @brief   Аппаратная карта CAN ID, пинов и пневмо-контуров стандарта N-Bus (v1.3)
  * @part    STM32G474RET6 (LQFP64) / Репозиторий: Nexus-OPTIC-HUB-UART
  * @note    Добавлена поддержка шины FDCAN3 и джампера переключения башен PB7.
  ********************************################################**************
  */

#ifndef DRIVER_VIRTUAL_MAP_H
#define DRIVER_VIRTUAL_MAP_H

#include "main.h"

/* ========================================================================== */
/* 1. ПОЛНАЯ МАТРИЦА АЛЛОКАЦИИ ИДЕНТИФИКАТОРОВ ШИНЫ N-BUS (СТАНДАРТ v1.3)     */
/* ========================================================================== */
#define CAN_ID_SYS_SYNC            0x010  // Глобальный тактовый синхроимпульс (Master)
#define CAN_ID_RT_CORRECTION       0x050  // Активная наносекундная коррекция траектории X/Y
#define CAN_ID_ACCEL_DATA          0x120  // Высокопрецизионный 20-бит поток вибрации головки

#define CAN_ID_DRIVE_X_CMD         0x201  // Уставка координаты/скорости оси X
#define CAN_ID_DRIVE_Y_CMD         0x202  // Уставка координаты/скорости оси Y
#define CAN_ID_DRIVE_Z1_CMD        0x203  // Целевая высота сопла Z1 (Поддержка до Z8)
#define CAN_ID_DRIVE_Z2_CMD        0x204  
#define CAN_ID_DRIVE_Z3_CMD        0x205  
#define CAN_ID_DRIVE_Z4_CMD        0x206  

#define CAN_ID_DRIVE_X_TELEMETRY   0x281  // Телеметрия Closed-Loop оси X
#define CAN_ID_DRIVE_Y_TELEMETRY   0x282  
#define CAN_ID_DRIVE_Z1_TELEMETRY  0x283  // Код инкрементального энкодера осей Z1-Z4
#define CAN_ID_DRIVE_Z2_TELEMETRY  0x284  
#define CAN_ID_DRIVE_Z3_TELEMETRY  0x285  
#define CAN_ID_DRIVE_Z4_TELEMETRY  0x286  

#define CAN_ID_TOWER_V_AXIS        0x310  // Нисходящий групповой макрос осей вращения
#define CAN_ID_TOWER_STATUS        0x311  // Локальный АЦП-вакуум присосок сопел (до 8 штук)
#define CAN_ID_ENV_DATA            0x410  // Температура SHT35 + Опорные давления P1/P2 мастера
#define CAN_ID_SERVICE_LOG         0x720  // Выгрузка Чёрного Ящика (W25Q128) при простое ЧПУ

#define CAN_ID_AUTH_CHALLENGE      0x700  // Кадр-запрос от привода-следователя к мастеру
#define CAN_ID_AUTH_RESPONSE       0x710  // Кадр-ответ от мастера к приводу-следователю

/* ========================================================================== */
/* 2. ТРИ ИЗОЛИРОВАННЫХ ТЕХНОЛОГИЧЕСКИХ ДОМЕНА СЕТИ N-BUS                     */
/* ========================================================================== */
#define HSE_CRYSTAL_FREQ_MHZ       24     // Внешний прецизионный кварц HSE хаба
#define CAN_PERIPH_CLK_MHZ         80     // Выровненная тактовая частота шин CAN после PLL

// ДОМЕН 1: МЕДНЫЙ СИЛОВОЙ ДОМЕН СТАНИНЫ (CAN FD1) - БРОНЕБОЙНЫЕ 4 МБИТ/С
#define COPPER_CAN_RX_PIN          GPIO_PIN_11 // PA11 (AF9)
#define COPPER_CAN_TX_PIN          GPIO_PIN_12 // PA12 (AF9)

// ДОМЕН 2: ОПТИЧЕСКИЙ ДОМЕН БАШНИ V6.0 (CAN FD2 / ИК-ЛУЧ) - РАЗГОН ДО 5 МБИТ/С
#define OPTIC_CAN_RX_PIN           GPIO_PIN_8  // PB8  (AF9)
#define OPTIC_CAN_TX_PIN           GPIO_PIN_9  // PB9  (AF9)

// ДОМЕН 3: ВСПОМОГАТЕЛЬНЫЙ МЕДНЫЙ ДОМЕН БАШНИ V3.0 (CAN FD3) - 4 МБИТ/С
#define TOWER3_CAN_RX_PIN          GPIO_PIN_0  // PA8  (AF11)
#define TOWER3_CAN_TX_PIN          GPIO_PIN_1  // PA15 (AF11)

/* ========================================================================== */
/* 3. АКТИВАТОРЫ, ДЖАМПЕРЫ КОНФИГУРАЦИИ И БЕЗОПАСНОСТИ                        */
/* ========================================================================== */
// Джампер 1 выбор конфигурации исполнительных башен (V6.0 по CAN FD2 / V3.0 по CAN FD3)
#define CONFIG_JUMPER_PORT         GPIOB
#define CONFIG_JUMPER_PIN          GPIO_PIN_7  // Ножка 55: PB7 (Внутренняя подтяжка Pull-Up)

// Аппаратный жесткий Board ID ревизии платы хаба головки
#define BOARD_ID_PORT              GPIOC
#define BOARD_ID_BIT0_PIN          GPIO_PIN_13 // Ножка 2:  PC13
#define BOARD_ID_BIT1_PIN          GPIO_PIN_14 // Ножка 3:  PC14

// Аппаратная секретка загрузчика (Security Boot-Key RC-замок Тау = 1 сек)
#define SECURE_KEY_ADC_PORT        GPIOA
#define SECURE_KEY_ADC_PIN         GPIO_PIN_4  // Ножка 20: PA4
#define SECURE_KEY_DIG_PORT        GPIOC
#define SECURE_KEY_DIG_PIN         GPIO_PIN_0  // Ножка 9:  PC0

// Выделенный диагностический выход луча синхронизации
#define DIAG_SYNC_OUT_PORT         GPIOB
#define DIAG_SYNC_OUT_PIN          GPIO_PIN_2  // Ножка 26: PB2

/* ========================================================================== */
/* 4. ДОМЕН ИЗМЕРИТЕЛЬНЫХ ДАТЧИКОВ ЛЕТАЮЩЕЙ БАШНИ ГОЛОВКИ                     */
/* ========================================================================== */
// Лазер Panasonic HG-C1030
#define HG_ADC_PORT                GPIOB
#define HG_ADC_PIN                 GPIO_PIN_1  // Ножка 18: ADC3_IN5
#define HG_EXT_INPUT_PORT          GPIOC
#define HG_EXT_INPUT_PIN           GPIO_PIN_6  // Ножка 37: GPIO_Output
#define HG_POWER_PORT              GPIOC
#define HG_POWER_PIN               GPIO_PIN_15 // Ножка 3:  GPIO_Output

// Выделенные однонаправленные линии приема потоковой телеметрии USART DMA
#define TOWER1_UART_RX_PORT        GPIOA
#define TOWER1_UART_RX_PIN         GPIO_PIN_10 // Ножка 43: USART1_RX (Башни 1-2)
#define TOWER2_UART_RX_PORT        GPIOB
#define TOWER2_UART_RX_PIN         GPIO_PIN_4  // Ножка 52: USART2_RX (Башни 3-4)

#endif /* DRIVER_VIRTUAL_MAP_H */
