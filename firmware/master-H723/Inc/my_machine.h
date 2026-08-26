#ifndef MY_MACHINE_H
#define MY_MACHINE_H

// ==============================================================================
// ГЛОБАЛЬНЫЙ ТУМБЛЕР КРИПТОЗАЩИТЫ (ON = 1 / OFF = 0)
// Отключайте на этапе отладки прототипа, включайте перед серийным производством!
// ==============================================================================
#define ENABLE_SECURITY_SUBSYSTEM   1  // 1 - Защита активна, 0 - Режим чистой отладки

// 1. Конфигурация базового ядра grblHAL (Честные оси планировщика)
#define N_AXIS 8
#define AXIS_NAMES "XYZUVR1R2" // X, Y, Z1..Z4, R1..R2 (Спаренное вращение)

// 2. РЕЗЕРВ РАСШИРЕНИЯ: Параметры виртуальных осей вращения сопел свыше 8 осей
#define VIRT_AXIS_COUNT          8       
#define R_STEPS_PER_DEGREE       8.8888f 
#define R_MAX_VELOCITY           500.0f  
#define R_MAX_ACCELERATION       5000.0f 

// 3. Адресация защищенных ячеек памяти внешней EEPROM 24LC16B
#define EEPROM_I2C_ADDR          0x50    
#define SEC_COUNTERFEIT_FLAG     0x0F00  // Адрес флага подделки платы
#define SEC_STARTUPS_LEFT_COUNT  0x0F04  // Адрес счетчика дней/запусков до саботажа

#endif // MY_MACHINE_H

