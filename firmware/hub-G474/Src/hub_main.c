*** Begin Patch
*** Update File: firmware/hub-G474/Src/hub_main.c
@@
-    HAL_GPIO_WritePin(READY_PORT, READY_PIN, GPIO_PIN_RESET);
-    // Принудительно выключаем белый светодиод оригинальности SECRET_STATUS
-    HAL_GPIO_WritePin(LED_PORT_C, LED_PIN_SECRET_STATUS, GPIO_PIN_SET);
+    HAL_GPIO_WRITE_SAFE(READY_PORT, READY_PIN, GPIO_PIN_RESET);
+    // Принудительно выключаем белый светодиод оригинальности SECRET_STATUS
+    HAL_GPIO_WRITE_SAFE(LED_PORT_C, LED_PIN_SECRET_STATUS, GPIO_PIN_SET);
*** End Patch
