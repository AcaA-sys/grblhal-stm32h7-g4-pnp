/**
  ********************************################################**************
  * @file    driver_virtual_map.h
  * @brief   Официальный заголовочный файл карты N-Bus (Стандарт v1.3)
  * @part    STM32G474CBTx (LQFP48) / Проект: grblhal-stm32h7-g4-pnp
  * @note    Автоматически синхронизирован с манифестом N-BUS.md
  ********************************################################**************
  */

#ifndef DRIVER_VIRTUAL_MAP_H
#define DRIVER_VIRTUAL_MAP_H

#include "main.h"

/* ========================================================================== */
/* 1. МАТРИЦА ПРИКЛАДНЫХ ИДЕНТИФИКАТОРОВ ШИНЫ N-BUS (СТАНДАРТ v1.3)          */
/* ========================================================================== */
#define CAN_ID_SYS_SYNC            0x010  /* Глобальный синхроимпульс такта */
#define CAN_ID_RT_CORRECTION       0x050  /* Офсет active-компенсации X/Y */
#define CAN_ID_ACCEL_DATA          0x120  /* 20-бит поток вибрации (ISM330) */

#define CAN_ID_DRIVE_X_CMD         0x201  /* Уставка координаты/скорости X */
#define CAN_ID_DRIVE_Y_CMD         0x202  /* Уставка координаты/скорости Y */
#define CAN_ID_DRIVE_Z1_CMD        0x203  /* Целевая высота сопла Z1 */
#define CAN_ID_DRIVE_Z2_CMD        0x204  /* Целевая высота сопла Z2 */
#define CAN_ID_DRIVE_Z3_CMD        0x205  /* Целевая высота сопла Z3 */
#define CAN_ID_DRIVE_Z4_CMD        0x206  /* Целевая высота сопла Z4 */

#define CAN_ID_DRIVE_X_TELEMETRY   0x281  /* actual_pos, following_error */
#define CAN_ID_DRIVE_Y_TELEMETRY   0x282  /* actual_pos, following_error */
#define CAN_ID_DRIVE_Z1_TELEMETRY  0x283  /* Код энкодера A/B/Z осей Z1 */
#define CAN_ID_DRIVE_Z2_TELEMETRY  0x284  /* Код энкодера A/B/Z осей Z2 */
#define CAN_ID_DRIVE_Z3_TELEMETRY  0x285  /* Код энкодера A/B/Z осей Z3 */
#define CAN_ID_DRIVE_Z4_TELEMETRY  0x286  /* Код энкодера A/B/Z осей Z4 */

#define CAN_ID_TOWER_V_AXIS        0x310  /* Групповой макрос осей R1/R2 */
#define CAN_ID_TOWER_STATUS        0x311  /* Оцифровка вакуума Vac_S1-S4 */
#define CAN_ID_ENV_DATA            0x410  /* Температура SHT35 + Опорные Давления */
#define CAN_ID_SERVICE_LOG         0x720  /* Вывод Чёрного Ящика (W25Q128) */

#define CAN_ID_AUTH_CHALLENGE      0x700  /* Проверочный кадр-запрос привода */
#define CAN_ID_AUTH_RESPONSE       0x710  /* Ответный крипто-кадр мастера */

/* ========================================================================== */
/* 2. СЕТЕВЫЕ ДОМЕНЫ И СИСТЕМНЫЕ ЧАСТОТЫ (ДЛЯ КОРПУСА LQFP48)               */
/* ========================================================================== */
#define HSE_CRYSTAL_FREQ_MHZ       25     /* Опорный такт транзита от мастера */
#define CAN_PERIPH_CLK_MHZ         80     /* Выровненная частота шин CAN FD */

/* Аппаратная аллокация портов на кристалле */
#define CONFIG_JUMPER_PORT         GPIOB
#define CONFIG_JUMPER_PIN          GPIO_PIN_7   /* Ножка 55: Джампер PB7 */

#define BOARD_ID_PORT              GPIOC
#define BOARD_ID_BIT0_PIN          GPIO_PIN_13  /* Пин PC13 ревизии платы */
#define BOARD_ID_BIT1_PIN          GPIO_PIN_14  /* Пин PC14 ревизии платы */

#define SECURE_KEY_DIG_PORT        GPIOC
#define SECURE_KEY_DIG_PIN         GPIO_PIN_0   /* Пин PC0 аналогового замка */

#define DIAG_SYNC_OUT_PORT         GPIOB
#define DIAG_SYNC_OUT_PIN          GPIO_PIN_2   /* Блик контроля джиттера READY */

#endif /* DRIVER_VIRTUAL_MAP_H */
