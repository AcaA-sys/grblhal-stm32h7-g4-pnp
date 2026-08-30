################################################################################
# ПРЕЦИЗИОННЫЙ БОЕВОЙ MAKEFILE СБОРКИ ПРОШИВКИ СТАНДАРТА N-BUS (v1.3.1)
# Целевой микроконтроллер: STM32G474CBT6 (Маршрутизатор, 170 МГц)
# Репозиторий: grblhal-stm32h7-g4-pnp
# Фикс: Прямой поиск hub_main.c в корне папки без виртуальных подкаталогов Src/
################################################################################

TARGET = nexus_hub_g474_firmware
OPT = -Os
BUILD_DIR = build

# Исходный Си-код лежит прямо в каталоге рядом с Makefile
C_SOURCES = hub_main.c

PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size

HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S
 
# Флаги архитектуры кристалла (Cortex-M4 + Hardware FPU)
MCU = -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16
C_DEFS = -DSTM32G474xx -DUSE_HAL_DRIVER

# Пути поиска заголовочных файлов (Текущая папка, папка Inc и временная build)
C_INCLUDES = -I. -IInc -Iinc -I$(BUILD_DIR)

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -c -ffreestanding

all: $(BUILD_DIR) $(BUILD_DIR)/driver_virtual_map.h $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

# АВТОГЕНЕРАЦИЯ ЗАКРЫТОЙ КАРТЫ ПИНОВ v1.3.1 СТРОГО В ПАПКУ BUILD/
$(BUILD_DIR)/driver_virtual_map.h:
	@echo "Принудительная генерация закрытого эталонного файла driver_virtual_map.h v1.3.1..."
	@echo '#ifndef DRIVER_VIRTUAL_MAP_H' > $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define DRIVER_VIRTUAL_MAP_H' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#include "main.h"' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_SYS_SYNC 0x010' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_RT_CORRECTION 0x050' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_ACCEL_DATA 0x120' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_X_CMD 0x201' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Y_CMD 0x202' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z1_CMD 0x203' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z2_CMD 0x204' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z3_CMD 0x205' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z4_CMD 0x206' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_X_TELEMETRY 0x281' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Y_TELEMETRY 0x282' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z1_TELEMETRY 0x283' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z2_TELEMETRY 0x284' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z3_TELEMETRY 0x285' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_DRIVE_Z4_TELEMETRY 0x286' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_TOWER_V_AXIS 0x310' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_TOWER_STATUS 0x311' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_ENV_DATA 0x410' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_SERVICE_LOG 0x720' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_AUTH_CHALLENGE 0x700' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CAN_ID_AUTH_RESPONSE 0x710' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define BOARD_ID_PORT GPIOC' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define BOARD_ID_BIT0_PIN GPIO_PIN_13' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define BOARD_ID_BIT1_PIN GPIO_PIN_14' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CONFIG_JUMPER_PORT GPIOB' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define CONFIG_JUMPER_PIN GPIO_PIN_7' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define SECURE_KEY_DIG_PORT GPIOC' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define SECURE_KEY_DIG_PIN GPIO_PIN_0' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define HG_POWER_PORT GPIOC' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define HG_POWER_PIN GPIO_PIN_15' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define HG_EXT_INPUT_PORT GPIOC' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define HG_EXT_INPUT_PIN GPIO_PIN_6' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define DIAG_SYNC_OUT_PORT GPIOB' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define DIAG_SYNC_OUT_PIN GPIO_PIN_2' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define TOWER1_UART_RX_PORT GPIOA' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define TOWER1_UART_RX_PIN GPIO_PIN_10' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define TOWER2_UART_RX_PORT GPIOB' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define TOWER2_UART_RX_PIN GPIO_PIN_4' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define SECURE_KEY_ADC_PORT GPIOA' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define SECURE_KEY_ADC_PIN GPIO_PIN_4' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define MIC_ADC_PORT GPIOB' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define MIC_DIFF_P_PIN GPIO_PIN_14' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define MIC_DIFF_N_PIN GPIO_PIN_15' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define IMU_CS_PORT GPIOC' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define IMU_CS_PIN GPIO_PIN_5' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define IMU_INT1_PORT GPIOB' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define IMU_INT1_PIN GPIO_PIN_0' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define IMU_INT2_PORT GPIOA' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define IMU_INT2_PIN GPIO_PIN_1' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define OPT_INT3_PORT GPIOC' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#define OPT_INT3_PIN GPIO_PIN_7' >> $(BUILD_DIR)/driver_virtual_map.h
	@echo '#endif' >> $(BUILD_DIR)/driver_virtual_map.h

$(BUILD_DIR)/$(TARGET).o: $(C_SOURCES) $(BUILD_DIR)/driver_virtual_map.h
	@echo "Запуск честной машинной компиляции кода маршрутизатора..."
	$(CC) $(C_SOURCES) $(CFLAGS) -o $(BUILD_DIR)/$(TARGET).o
	$(SZ) $(BUILD_DIR)/$(TARGET).o

$(BUILD_DIR)/$(TARGET).hex: $(BUILD_DIR)/$(TARGET).o
	$(HEX) $(BUILD_DIR)/$(TARGET).o $(BUILD_DIR)/$(TARGET).hex

$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).hex
	$(BIN) $(BUILD_DIR)/$(TARGET).o $(BUILD_DIR)/$(TARGET).bin
	@echo "Архитектурная сборка N-Bus v1.3.1 успешно завершена!"

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -fR $(BUILD_DIR)
  
.PHONY: all clean


