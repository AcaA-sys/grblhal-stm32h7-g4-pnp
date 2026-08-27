*** Begin Patch
*** Update File: firmware/master-H723/Security/sec_crypto.c
@@
-            simulated_backlash_x = 0; // Ось X едет идеально
-packet->positions = raw_pos + simulated_backlash_y;
-break;
+            simulated_backlash_x = 0; // Ось X едет идеально
+            /* Исправление: ранее была попытка присвоить указатель массиву.
+             * Нужно корректно записать модифицированную позицию оси Y. */
+            packet->positions[1] = raw_pos[1] + simulated_backlash_y;
+            break;
@@
-/* Сохраняем уставки текущего такта для анализа направления на следующем шаге через 0.5 мс */
-last_pos_x = raw_pos[0];
-last_pos_y = raw_pos;
+/* Сохраняем уставки текущего такта для анализа направления на следующем шаге через 0.5 мс */
+last_pos_x = raw_pos[0];
+last_pos_y = raw_pos[1];
*** End Patch
