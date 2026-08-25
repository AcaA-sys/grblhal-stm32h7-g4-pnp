#ifndef MY_MACHINE_H
#define MY_MACHINE_H

// 1. Конфигурация базового ядра grblHAL (Честные оси планировщика)
#define N_AXIS 8
#define AXIS_NAMES "XYZUVR1R2" // X, Y, Z1..Z4 (U,V), R1..R2 (Спаренное вращение)

// 2. РЕЗЕРВ РАСШИРЕНИЯ: Параметры виртуальных осей вращения сопел свыше 8 осей
// (Железная настройка кинематики головок, скрытая от ioSender)
#define VIRT_AXIS_COUNT          8       // Дополнительные сопла R1-R8 при переключении
#define R_STEPS_PER_DEGREE       8.8888f // 1/16 микрошаг, моторы NEMA11, редукция 1:1
#define R_MAX_VELOCITY           500.0f  // Максимальная скорость вращения пипетки (град/сек)
#define R_MAX_ACCELERATION       5000.0f // Максимальное софт-ускорение (град/сек²)

// 3. Адресация защищенных ячеек памяти внешней EEPROM 24LC16B
#define EEPROM_I2C_ADDR          0x50    // Базовый адрес первого блока памяти 24LC16B
#define SEC_COUNTERFEIT_FLAG     0x0F00  // Флаг обнаружения пиратского клона платы
#define SEC_STARTUPS_LEFT_COUNT  0x0F04  // Счетчик отсрочки до запуска карусели саботажа

#endif // MY_MACHINE_H
