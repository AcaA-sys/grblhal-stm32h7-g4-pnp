
```text
/* USER CODE BEGIN 4 */
void Nexus_Router_Decode_And_Transmit(uint8_t *spi_rx_buffer)
{
    FDCAN_TxHeaderTypeDef TxHeader;
    TxHeader.IdType             = FDCAN_STANDARD_ID;
    TxHeader.TxFrameType        = FDCAN_DATA_FRAME;
    TxHeader.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    TxHeader.BitRateSwitch      = FDCAN_BRS_ON;
    TxHeader.FDFormat           = FDCAN_FD_CAN;
    TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    TxHeader.MessageMarker      = 0;

    // Первый байт открытого SPI-пакета — это скрытый код-маркер команды (Command Token)
    uint8_t command_token = spi_rx_buffer[0]; 

    switch(command_token)
    {
```

        // ====================================================================
        // ⚡ ДОМЕН CAN FD1: СИЛОВОЙ КОНТУР ПРИВОДОВ СТАНИНЫ (X / Y)
        // ====================================================================
        case 0x01: // Уставка движения координаты X
            TxHeader.Identifier = 0x201; // Скрытый ID: DRIVE_X_CMD
            TxHeader.DataLength = FDCAN_DLC_BYTES_4;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, &spi_rx_buffer[1]);
            break;

        case 0x02: // Уставка движения координаты Y
            TxHeader.Identifier = 0x202; // Скрытый ID: DRIVE_Y_CMD
            TxHeader.DataLength = FDCAN_DLC_BYTES_4;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &TxHeader, &spi_rx_buffer[1]);
            break;

        // ====================================================================
        // 📡 ДОМЕН CAN FD2: ИЗМЕРИТЕЛЬНЫЙ ИСПОЛНИТЕЛЬНЫЙ КОНТУР ГОЛОВКИ (ОПТИКА)
        // ====================================================================
        case 0x11: // Синхроимпульс тактовой сетки ФАПЧ
            TxHeader.Identifier = 0x010; // Скрытый ID: SYS_SYNC
            TxHeader.DataLength = FDCAN_DLC_BYTES_2;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, &spi_rx_buffer[1]);
            break;

        case 0x15: // Высокоприоритетный офсет гашения резонансов по ИИ-логу
            TxHeader.Identifier = 0x050; // Скрытый ID: RT_CORRECTION
            TxHeader.DataLength = FDCAN_DLC_BYTES_4;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, &spi_rx_buffer[1]);
            break;

        case 0x21: // Уставки положения вертикальных осей подлета Z1-Z4
            // Маршрутизатор сам вычисляет ID (0x203..0x206) на основе номера оси из SPI!
            TxHeader.Identifier = 0x203 + spi_rx_buffer[1]; // Смещаемся на номер оси Z
            TxHeader.DataLength = FDCAN_DLC_BYTES_4;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, &spi_rx_buffer[2]);
            break;

        case 0x25: // Групповой макрос уставок вращения насадок R1-R8
            TxHeader.Identifier = 0x241 + spi_rx_buffer[1]; // Смещаемся на номер оси R
            TxHeader.DataLength = FDCAN_DLC_BYTES_4;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, &spi_rx_buffer[2]);
            break;

        case 0x31: // Приказ на изменение состояния клапанов вакуума Pro-башни V6.0
            TxHeader.Identifier = 0x311; // Скрытый ID: TOWER_STATUS
            TxHeader.DataLength = FDCAN_DLC_BYTES_20; // 20-байтный монолит N-BUS v1.4
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &TxHeader, &spi_rx_buffer[1]);
            break;

        // ====================================================================
        // 📡 ДОМЕН CAN FD3: АВТОНОМНЫЙ МЕДНЫЙ КОНТУР МЛАДШЕЙ БАШНИ СОПЕЛ
        // ====================================================================
        case 0xA0: // Команда управления клапанами и шагами Варианта В (Tower V3.0)
            TxHeader.Identifier = 0x211; // Скрытый ID: TOWER_V3_CMD
            TxHeader.DataLength = FDCAN_DLC_BYTES_4;
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan3, &TxHeader, &spi_rx_buffer[1]);
            break;

        default:
            // Фильтрация: Неизвестный токен отбрасывается, защищая шину от мусора
            break;
    }
}
/* USER CODE END 4 */
////////////
🛡️ ЧЕМ ЭТО ПРЕВОСХОДИТ СТАНДАРТНЫЙ ПОДХОД:
1.	Полная изоляция (Decoupling): В открытом кодовой базе grblHAL для STM32H7 теперь нет ни одного упоминания о секретных идентификаторах 0x010, 0x050, 0x120, 0x241 или 0x311. Вся коммерческая тайна намертво зашита внутри бинарника Маршрутизатора G474.
2.	Динамическое автовычисление: Мастеру больше не нужно плодить кучу функций под каждую пипетку — он просто пишет в SPI: «команда 0x25 (вращение R), ось №3, угол 45°». Маршрутизатор принимает этот пакет и сам мгновенно пересчитывает его в секретный CAN ID 0x244 (0x241 + 3), автоматически направляя его в оптический передатчик FDCAN2.
