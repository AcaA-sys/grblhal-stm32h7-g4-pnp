/**
  ********************************################################**************
  * @file    driver_virtual_map.h
  * @brief   Аппаратная карта CAN ID, пинов  стандарта N-Bus (v1.3)
  * @part    STM32H723VBT6 (LQFP-100) 
  ********************************################################**************
  */

#if N_ABC_MOTORS > 8
#error "Nexus-H7-FD."
#endif

#if !(defined(STM32H723xx)) || HSE_VALUE != 25000000
#error "This board has a STM32H723 processor with 25MHz crystal, select a corresponding build!"
#endif

#ifndef DRIVER_VIRTUAL_MAP_H
#define DRIVER_VIRTUAL_MAP_H

#include "main.h"

/* ========================================================================== */
/* 1. ПОЛНАЯ МАТРИЦА АЛЛОКАЦИИ ИДЕНТИФИКАТОРОВ ШИНЫ N-BUS (СТАНДАРТ v1.3)     */
/* ========================================================================== */
//#define CAN_ID_SYS_SYNC            0x010  // Глобальный тактовый синхроимпульс (Master)
//#define CAN_ID_RT_CORRECTION       0x050  // Активная наносекундная коррекция траектории X/Y
//#define CAN_ID_ACCEL_DATA          0x120  // Высокопрецизионный 20-бит поток вибрации головки

#define CAN_ID_DRIVE_X_BASE         0x201  // Уставка координаты/скорости оси X
#define CAN_ID_DRIVE_Y_BASE         0x202  // Уставка координаты/скорости оси Y
#define CAN_ID_DRIVE_Z1_BASE        0x203  // Целевая высота сопла Z1 (Поддержка до Z8) Z1 = 0x203, Z2 = 0x204, Z3 = 0x205, Z4 = 0x206 ... до Z8
#define CAN_ID_DRIVE_Z2_BASE        0x204  
#define CAN_ID_DRIVE_Z3_BASE        0x205  
#define CAN_ID_DRIVE_Z4_BASE        0x206  

#define CAN_ID_DRIVE_R1_BASE     0x241  //R1 = 0x241, R2 = 0x242, R3 = 0x243 ... до R8
#define CAN_ID_DRIVE_R2_BASE     0x242
#define CAN_ID_DRIVE_R3_BASE     0x243
#define CAN_ID_DRIVE_R4_BASE     0x244
#define CAN_ID_DRIVE_R5_BASE     0x245
#define CAN_ID_DRIVE_R6_BASE     0x246
#define CAN_ID_DRIVE_R7_BASE     0x247
#define CAN_ID_DRIVE_R8_BASE     0x248

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
/* 2. ТРИ ИЗОЛИРОВАННЫХ ТЕХНОЛОГИЧЕСКИХ ДОМЕНА СЕТИ N-BUS   hub-G474          */
/* ========================================================================== */
#define HSE_CRYSTAL_FREQ_MHZ       25     // Мастер (H723, 550 МГц) и Слейв (G474, 170 МГц) синхронизированы: кварц 25 МГц на мастере, выход MCO1 (PA8) -> HSE Bypass (PF0) на хабе
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
/* 3. АКТИВАТОРЫ, ДЖАМПЕРЫ КОНФИГУРАЦИИ И БЕЗОПАСНОСТИ    hub-G474            */
/* ========================================================================== */
// Джампер 1 выбор конфигурации исполнительных башен (V6.0 по CAN FD2 / V3.0 по CAN FD3)
#define CONFIG_JUMPER1_PORT         GPIOB
#define CONFIG_JUMPER1_PIN          GPIO_PIN_6  // Ножка 55: PB6 (Внутренняя подтяжка Pull-Up)
#define CONFIG_JUMPER2_PORT         GPIOB
#define CONFIG_JUMPER2_PIN          GPIO_PIN_7  // Ножка 55: PB7 (Внутренняя подтяжка Pull-Up)
#define CONFIG_JUMPER3_PORT         GPIOB
#define CONFIG_JUMPER3_PIN          GPIO_PIN_9  // Ножка 55: PB9 (Внутренняя подтяжка Pull-Up)
#define CONFIG_JUMPER4_PORT         GPIOC
#define CONFIG_JUMPER4_PIN          GPIO_PIN_14  // Ножка 55: PC14 (Внутренняя подтяжка Pull-Up)


// Аппаратная секретка загрузчика (Security Boot-Key )
#define SECURE_KEY_ADC_PORT        GPIOB
#define SECURE_KEY_ADC_PIN         GPIO_PIN_11  // Ножка 20: PB11
#define SECURE_KEY_DIG_PORT        GPIOB
#define SECURE_KEY_DIG_PIN         GPIO_PIN_10  // Ножка 9:  PB10

// Выделенный  выход READY
#define DIAG_SYNC_OUT_PORT         GPIOB
#define DIAG_SYNC_OUT_PIN          GPIO_PIN_2  // Ножка 26: PB2

/////////////////
// Define user-control controls (cycle start, reset, feed hold) input pins.
#if CONTROL_ENABLE & CONTROL_HALT
#define RESET_PORT                  AUXINPUT2_PORT
#define RESET_PIN                   AUXINPUT2_PIN
#endif

#define AUXINPUT0_ANALOG_PORT   GPIOA  // ANALOG //Src\driver.c   Строка  209:
#define AUXINPUT0_ANALOG_PIN    0
#define AUXINPUT1_ANALOG_PORT   GPIOA // ANALOG
#define AUXINPUT1_ANALOG_PIN    1
#define AUXINPUT2_ANALOG_PORT   GPIOA  // ANALOG
#define AUXINPUT2_ANALOG_PIN    2
#define AUXINPUT3_ANALOG_PORT   GPIOA // ANALOG
#define AUXINPUT3_ANALOG_PIN    3
#define AUXINPUT4_ANALOG_PORT   GPIOC  // ANALOG
#define AUXINPUT4_ANALOG_PIN    4
#define AUXINPUT5_ANALOG_PORT   GPIOC // ANALOG
#define AUXINPUT5_ANALOG_PIN    5

// Spindle PWM 
#define AUXOUTPUT0_PORT             GPIOE   // - FAN0
#define AUXOUTPUT0_PIN              9

#define AUXOUTPUT1_PORT             GPIOE   // SPINDLE_PWM
#define AUXOUTPUT1_PIN              14

#define AUXOUTPUT2_PORT             GPIOC   // SPINDLE_ENABLE
#define AUXOUTPUT2_PIN              -1

#define AUXOUTPUT3_PORT             GPIOC   // 
#define AUXOUTPUT3_PIN              -1
/* ========================================================================== */
/* 4. ДОМЕН ИЗМЕРИТЕЛЬНЫХ ДАТЧИКОВ ЛЕТАЮЩЕЙ БАШНИ ГОЛОВКИ                     */
/* ========================================================================== */

// Битовые маски управления исполнительной пневматикой Башни 1 (Сопла 1-4)
#define VALVE_T1_VAC_S1        (1 << 0)  // 0x01: Включить вакуум Сопла 1
#define VALVE_T1_BLOW_S1       (1 << 1)  // 0x02: Импульс сброса Сопла 1
#define VALVE_T1_VAC_S2        (1 << 2)  // 0x04: Включить вакуум Сопла 2
#define VALVE_T1_BLOW_S2       (1 << 3)  // 0x08: Импульс сброса Сопла 2
#define VALVE_T1_VAC_S3        (1 << 4)  // 0x01: Включить вакуум Сопла 1
#define VALVE_T1_BLOW_S3       (1 << 5)  // 0x02: Импульс сброса Сопла 1
#define VALVE_T1_VAC_S4        (1 << 6)  // 0x04: Включить вакуум Сопла 2
#define VALVE_T1_BLOW_S4       (1 << 7)  // 0x08: Импульс сброса Сопла 2
#define VALVE_T1_VAC_S5        (1 << 8)  // 0x01: Включить вакуум Сопла 1
#define VALVE_T1_BLOW_S5       (1 << 9)  // 0x02: Импульс сброса Сопла 1
#define VALVE_T1_VAC_6        (1 << 10)  // 0x04: Включить вакуум Сопла 2
#define VALVE_T1_BLOW_S6       (1 << 11)  // 0x08: Импульс сброса Сопла 2
#define VALVE_T1_VAC_S7        (1 << 12)  // 0x01: Включить вакуум Сопла 1
#define VALVE_T1_BLOW_S7       (1 << 13)  // 0x02: Импульс сброса Сопла 1
#define VALVE_T1_VAC_S8        (1 << 14)  // 0x04: Включить вакуум Сопла 2
#define VALVE_T1_BLOW_S8      (1 << 15)  // 0x08: Импульс сброса Сопла 2


// Маска флагов Z-Index (Физический нуль вертикальных осей башни)
#define LIMIT_Z1_HOME_BIT      (1 << 0)  // Сработал концевик оси Z1
#define LIMIT_Z2_HOME_BIT      (1 << 1)  // Сработал концевик оси Z2
#define LIMIT_Z3_HOME_BIT      (1 << 2)  // Сработал концевик оси Z3
#define LIMIT_Z4_HOME_BIT      (1 << 3)  // Сработал концевик оси Z4
#define LIMIT_X_HOME_BIT      (1 << 4)  // Сработал концевик оси X
#define LIMIT_Y_HOME        // Сработал концевик оси Y


// Битовые маркеры сервисной периферии (LED-подсветка головы)
#define LED_MASK_MAIN_LIGHT    (1 << 0)  // 0x01: Включить бестеневой прожектор сопел
#define LED_MASK_CAMERA_LASER  (1 << 1)  // 0x02: Активировать лазерный прицел центровки камеры

// Выделенные однонаправленные линии приема потоковой телеметрии USART DMA
#define TOWER1_UART_RX_PORT        GPIOA
#define TOWER1_UART_RX_PIN         GPIO_PIN_10 // Ножка 43: USART1_RX (Башни 1-2)
#define TOWER2_UART_RX_PORT        GPIOB
#define TOWER2_UART_RX_PIN         GPIO_PIN_4  // Ножка 52: USART2_RX (Башни 3-4)

#endif /* DRIVER_VIRTUAL_MAP_H */


// Аппаратный жесткий Board ID ревизии платы OPTIC-HUB
//#define BOARD_ID_PORT              GPIOC
//#define BOARD_ID_BIT0_PIN          GPIO_PIN_13 // Ножка 2:  PC13
//#define BOARD_ID_BIT1_PIN          GPIO_PIN_14 // Ножка 3:  PC14
