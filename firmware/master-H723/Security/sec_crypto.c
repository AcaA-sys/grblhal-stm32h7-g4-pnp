*** Begin Patch
*** Update File: firmware/master-H723/Security/sec_crypto.c
@@
-        HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_RESET);
+        HAL_GPIO_WRITE_SAFE(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_RESET);
@@
-        HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
+        HAL_GPIO_WRITE_SAFE(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
*** End Patch
