// Внутри файла sec_crypto.c
void check_hardware_authenticity(void) {
#if ENABLE_SECURITY_SUBSYSTEM
    // Здесь живет весь наш суровый код:
    // Выдача синусоиды с ЦАП PA4, чтение АЦП PA5, проверка EEPROM и запуск карусели
    if (is_counterfeit) {
        activate_backlash_sabotage();
    }
#else
    // В режиме отладки (0) этот блок пуст. 
    // Плата всегда считается оригинальной, станок едет идеально со стопроцентной точностью.
    is_counterfeit = false;
#endif
}

