/*
  driver_virtual_map.h - Карта пинов для Full-Digital режима Мастера Nexus-H7-FD
  Все оси моторов назначены ВИРТУАЛЬНЫМИ (GPIO_NUM_NC). 
  Физические разъемы двигателей на плате отсутствуют.
 * MCU: STM32H723VGT6 (550MHz)
 * Target: grblHAL / OpenPnP
 * 
 * Автор: AcaA-sys
*/

#ifndef _DRIVER_VIRTUAL_MAP_H_
#define _DRIVER_VIRTUAL_MAP_H_

#include "stm32h7xx_hal.h"

/*
 * Минимальные совместимые замены/защиты макросов, если они не определены
 */
#ifndef GPIO_NUM_NC
#define GPIO_NUM_NC 0xFFFFU
#endif

// 1. ТАКТИРОВАНИЕ ( 25МГц)
#define HSE_VALUE 25000000UL

// 1. КОНФИГУРАЦИЯ КОЛИЧЕСТВА ОСЕЙ
// Объявляем стандартные 8 оси для OpenPnP (X, Y, Z и ось вращения сопла C)
#define N_AXIS 8

// 2. ВИРТУАЛИЗАЦИЯ ОСЕЙ ДВИЖЕНИЯ (Step / Dir)
// Назначаем константу GPIO_NUM_NC. Кремниевые выводы процессора остаются свободными.
#define STEP_PORT       NULL
#define DIR_PORT        NULL

#define X_STEP_PIN      GPIO_NUM_NC
#define X_DIR_PIN       GPIO_NUM_NC

#define Y_STEP_PIN      GPIO_NUM_NC
#define Y_DIR_PIN       GPIO_NUM_NC

#define Z_STEP_PIN      GPIO_NUM_NC  // Z1_STEP
#define Z_DIR_PIN       GPIO_NUM_NC

#define A_STEP_PIN      GPIO_NUM_NC  // Z2_STEP
#define A_DIR_PIN       GPIO_NUM_NC

#define B_STEP_PIN      GPIO_NUM_NC  // Z3_STEP
#define B_DIR_PIN       GPIO_NUM_NC

#define C_STEP_PIN      GPIO_NUM_NC  // Z4_STEP
#define C_DIR_PIN       GPIO_NUM_NC

#define U_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define U_DIR_PIN       GPIO_NUM_NC

#define W_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define W_DIR_PIN       GPIO_NUM_NC

#define P_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define P_DIR_PIN       GPIO_NUM_NC

#define Q_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define Q_DIR_PIN       GPIO_NUM_NC

#define V_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define V_DIR_PIN       GPIO_NUM_NC

#define R_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define R_DIR_PIN       GPIO_NUM_NC

#define D_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define D_DIR_PIN       GPIO_NUM_NC

#define E_STEP_PIN      GPIO_NUM_NC  // ось C (Rotation)
#define E_DIR_PIN       GPIO_NUM_NC

// 3. ВИРТУАЛИЗАЦИЯ СИГНАЛОВ УПРАВЛЕНИЯ ДРАЙВЕРАМИ (Enable)
// Сигналы EN также уходят в виртуальную память и будут передаваться цифровым флагом по SPI2
#define X_ENABLE_PIN    GPIO_NUM_NC
#define Y_ENABLE_PIN    GPIO_NUM_NC
#define Z_ENABLE_PIN    GPIO_NUM_NC
#define A_ENABLE_PIN    GPIO_NUM_NC
#define B_ENABLE_PIN    GPIO_NUM_NC
#define C_ENABLE_PIN    GPIO_NUM_NC
#define U_ENABLE_PIN    GPIO_NUM_NC
#define W_ENABLE_PIN    GPIO_NUM_NC
#define P_ENABLE_PIN    GPIO_NUM_NC
#define Q_ENABLE_PIN    GPIO_NUM_NC
#define V_ENABLE_PIN    GPIO_NUM_NC
#define R_ENABLE_PIN    GPIO_NUM_NC
#define D_ENABLE_PIN    GPIO_NUM_NC
#define E_ENABLE_PIN    GPIO_NUM_NC

// 4. ФИЗИЧЕСКИЙ МЕЖЧИПОВЫЙ ИНТЕРФЕЙС SPI2_ ДЛЯ СВЯЗИ С СОПРОЦЕССОРОМ NGW-FD
// Эти пины жестко разводятся на плате Nexus-H7-FD короткими дорожками до STM32G474
#define SPI2_PORT       GPIOB
#define SPI2_NSS_PIN    GPIO_PIN_12   // PB12 -> SPI2_NSS (Hardware Chip Select Slave)
#define SPI2_SCK_PIN    GPIO_PIN_13   // PB13 -> SPI2_SCK (Master Clock)
#define SPI2_MISO_PIN   GPIO_PIN_14   // PB14 -> SPI2_MISO (Master Input / Slave Output)
#define SPI2_MOSI_PIN   GPIO_PIN_15   // PB15 -> SPI2_MOSI (Master Output / Slave Input)

// 5. ФИЗИЧЕСКАЯ ЛИНИЯ АППАРАТНОЙ АВАРИИ (Hardware Interlock)
// Единственный силовой провод безопасности. Сюда заводится выход HW_E-STOP от Сопроцессора
#define HARDWARE_RESET_PORT  GPIOA
#define HARDWARE_RESET_PIN   GPIO_PIN_0   // GPIO_EXTI PA0 -> Жесткое EXTI прерывание сброса планировщика ЧПУ

// 6. ФИЗИЧЕСКИЕ КОНЦЕВИКИ СТАНИНЫ (X и Y)
// Концевики портала остаются физическими на плате Мастера для безопасного хоуминга базы
//#define X_LIMIT_PORT    GPIOA
//#define X_LIMIT_PIN     GPIO_PIN_1    // GPIO_EXTI PA1 -> Физический концевик оси X
#define Y_LIMIT_PORT    GPIOC  //GPIO_EXTI
#define Y_LIMIT_PIN     GPIO_PIN_4    // PC4 -> Физический концевик оси Y
//#define **_LIMIT_PORT    GPIOB
//#define **_LIMIT_PIN     GPIO_PIN_2    // GPIO_EXTI PB2 -> **

// 7. Концевик оси Z на Мастере отсутствует (концевики сопел физически опрашиваются на башне по FDCAN2)
#define X_LIMIT_PIN_NO  GPIO_NUM_NC
#define Z1_LIMIT_PIN     GPIO_NUM_NC
#define Z2_LIMIT_PIN     GPIO_NUM_NC
#define Z3_LIMIT_PIN     GPIO_NUM_NC
#define Z4_LIMIT_PIN     GPIO_NUM_NC

// 8. Блокировка драйверов (Безопасность)
#define UCC_DIS_PORT GPIOB
#define UCC_DIS_PIN  GPIO_PIN_10 // PB10 (Высокий уровень = Отключено)

// 9. СВЯЗЬ И ИНТЕРФЕЙСЫ
// RS-485 (USART2) NSIP93086HV-DSWR
#define RS485_PORT GPIOD
#define RS485_TX_PIN GPIO_PIN_5 // PD5
#define RS485_RX_PIN GPIO_PIN_6 // PD6
#define RS485_DE_PIN GPIO_PIN_4 // PD4 (управление направлением)

// 9.1 (USART3)
#define HMI_PORT GPIOD
#define HMI_TX_PIN GPIO_PIN_8 // PD8
#define HMI_RX_PIN GPIO_PIN_9 // PD9

// 10. АНАЛОГОВЫЕ ВХОДЫ (ADC)
#define ADC_PORT GPIOA
#define VACUUM1_PIN GPIO_PIN_2 // PA2 (ADC1_IN14) / XGZP6847  10K/20K
#define VACUUM2_PIN GPIO_PIN_3 // PA3 (ADC1_IN15) / XGZP6847  10K/20K
//#define ADC_PORT GPIOC
//#define VACUUM3_PIN  GPIO_PIN_0 // PC0 (ADC) / XGZP6847  10K/20K
//#define VACUUM4_PIN  GPIO_PIN_1 // PC1 (ADC) / XGZP6847  10K/20K
#define VBUS_DET_PIN GPIO_PIN_9 // PA9 (USB Sense) / 10K/10K

// 10.1 MPG (аналоговый и энкодер)
#define JOY_ADC_PORT GPIOB
#define JOY_SCALE_PIN GPIO_PIN_0 // PB0 (ADC1_IN18) Аналоговый статус галетника множителей / 5.1K/10K
#define JOY_AXIS_PIN GPIO_PIN_1 // PB1 (ADC1_IN19)  Аналоговый статус галетника осей / 5.1K/10K

#define JOY_ENCODER_PORT GPIOE
#define JOY_A_PIN GPIO_PIN_9 // PE9 ──► Аппаратный вход TIM1_CH1 (Фаза A маховика MPG)
#define JOY_B_PIN GPIO_PIN_11 // PE11 ──► Аппаратный вход TIM1_CH2 (Фаза B маховика MPG)

// 11. ПАМЯТЬ 24LC16B/FM24CL16B
#define I2C1_PORT GPIOB
#define I2C1_SCL_PIN GPIO_PIN_8 // PB8
#define I2C1_SDA_PIN GPIO_PIN_9 // PB9
#define I2C1_WP_PIN GPIO_PIN_7 // PB7

// 12. СИЛОВАЯ ПЕРИФЕРИЯ (PWM) - Порт E / UCC21520DW + NTMFS5C628
#define AUX_PWM_PORT GPIOE
#define OUT1_PIN GPIO_PIN_12 // PE12 (TIM1_CH3N)
#define OUT2_PIN GPIO_PIN_13 // PE13 (TIM1_CH3)
#define OUT3_PIN GPIO_PIN_14 // PE14 (TIM1_CH4)
#define OUT4_PIN GPIO_PIN_15 // PE15 (TIM1_BKIN2)

//#define AUXOUTPUT5_PORT GPIOE // оптрон АТС / 74LVC2G07 + TLP291
//#define AUXOUTPUT6_PORT GPIOE // оптрон АТС / 74LVC2G07 + TLP291

//#define LED_PORT GPIOC    // 74LVC2G07
//#define STATUS_LED_PIN GPIO_PIN_2 // PC2
//#define ERROR_LED_PIN GPIO_PIN_3 // PC3

// 13. СПЕЦИАЛЬНЫЕ НАСТРОЙКИ (grblHAL)
// Отключение детекции VBUS на PA9 
#define USB_VBUS_DETECTION_DISABLED

#endif /* _DRIVER_VIRTUAL_MAP_H_ */
