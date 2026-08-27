*** Begin Patch
*** Update File: firmware/master-H723/Src/main.c
@@
-    HAL_GPIO_Init(READY_PORT, &GPIO_InitStruct);
+    HAL_GPIO_INIT_SAFE(READY_PORT, &GPIO_InitStruct);
@@
-    HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0); 
+    HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0); 
     HAL_NVIC_EnableIRQ(EXTI3_IRQn);
@@
-    HAL_GPIO_Init(EEPROM_WP_PORT, &GPIO_InitStruct);
+    HAL_GPIO_INIT_SAFE(EEPROM_WP_PORT, &GPIO_InitStruct);
@@
-    HAL_GPIO_WritePin(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
+    HAL_GPIO_WRITE_SAFE(EEPROM_WP_PORT, EEPROM_WP_PIN, GPIO_PIN_SET);
*** End Patch
