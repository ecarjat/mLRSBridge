#include "stm_pro_mode.h"
#include "esp_task_wdt.h"

static const char *TAG_STM_PRO = "stm_pro_mode";

void endConn(void)
{
    logI(TAG_STM_PRO, "%s", "Ending Connection");
}

void setupSTM(uint8_t nb_uart)
{

    int timeout = 1000;
    while (!cmdSync(nb_uart) && timeout > 0)
    {
        esp_task_wdt_reset();
        vTaskDelay(1 / portTICK_PERIOD_MS);
       
        timeout--;
    }
    esp_task_wdt_reset();
    cmdGet(nb_uart);
    esp_task_wdt_reset();
    cmdVersion(nb_uart);
    cmdId(nb_uart);
    cmdErase(nb_uart);
    cmdExtErase(nb_uart);
}

int cmdSync(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "SYNC");

    char bytes[] = {0x7F};
    int resp = 1;
    return sendBytes(nb_uart, bytes, sizeof(bytes), resp);
}

int cmdGet(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "GET");

    char bytes[] = {0x00, 0xFF};
    int resp = 15;
    return sendBytes(nb_uart, bytes, sizeof(bytes), resp);
}

int cmdVersion(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "GET VERSION & READ PROTECTION STATUS");

    char bytes[] = {0x01, 0xFE};
    int resp = 5;
    return sendBytes(nb_uart, bytes, sizeof(bytes), resp);
}

int cmdId(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "CHECK ID");
    char bytes[] = {0x02, 0xFD};
    int resp = 5;
    return sendBytes(nb_uart, bytes, sizeof(bytes), resp);
}

int cmdErase(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "ERASE MEMORY");
    char bytes[] = {0x43, 0xBC};
    int resp = 1;
    int a = sendBytes(nb_uart, bytes, sizeof(bytes), resp);

    if (a == 1)
    {
        char params[] = {0xFF, 0x00};
        resp = 1;

        return sendBytes(nb_uart, params, sizeof(params), resp);
    }
    return 0;
}

int cmdExtErase(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "EXTENDED ERASE MEMORY");
    char bytes[] = {0x44, 0xBB};
    int resp = 1;
    int a = sendBytes(nb_uart, bytes, sizeof(bytes), resp);

    if (a == 1)
    {
        char params[] = {0xFF, 0xFF, 0x00};
        resp = 1;

        return sendBytes(nb_uart, params, sizeof(params), resp);
    }
    return 0;
}

int cmdWrite(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "WRITE MEMORY");
    char bytes[2] = {0x31, 0xCE};
    int resp = 1;
    return sendBytes(nb_uart, bytes, sizeof(bytes), resp);
}

int cmdRead(uint8_t nb_uart)
{
    logI(TAG_STM_PRO, "%s", "READ MEMORY");
    char bytes[2] = {0x11, 0xEE};
    int resp = 1;
    return sendBytes(nb_uart, bytes, sizeof(bytes), resp);
}

int loadAddress(uint8_t nb_uart, const char adrMS, const char adrMI, const char adrLI, const char adrLS)
{
    char x_or = adrMS ^ adrMI ^ adrLI ^ adrLS;
    char params[] = {adrMS, adrMI, adrLI, adrLS, x_or};
    int resp = 1;

    // ESP_LOG_BUFFER_HEXDUMP("LOAD ADDR", params, sizeof(params), ESP_LOG_DEBUG);
    return sendBytes(nb_uart, params, sizeof(params), resp);
}

int sendBytes(uint8_t nb_uart, const char *bytes, int count, int resp)
{
    sendData(nb_uart, TAG_STM_PRO, bytes, count);
    int length = waitForSerialData(nb_uart,resp, SERIAL_TIMEOUT);

    if (length > 0)
    {
        uint8_t data[length];
        const int rxBytes = uart_read_bytes(nb_uart, data, length, 1000 / portTICK_RATE_MS);

        if (rxBytes > 0 && data[0] == ACK)
        {
            logI(TAG_STM_PRO, "%s", "Sync Success");
            // ESP_LOG_BUFFER_HEXDUMP("SYNC", data, rxBytes, ESP_LOG_DEBUG);
            return 1;
        }
        else
        {
            logE(TAG_STM_PRO, "%s", "Sync Failure");
            return 0;
        }
    }
    else
    {
        logE(TAG_STM_PRO, "%s", "Serial Timeout");
        return 0;
    }

    return 0;
}

int sendData(uint8_t nb_uart, const char *logName, const char *data, const int count)
{
    const int txBytes = uart_write_bytes(nb_uart, data, count);
    //logD(logName, "Wrote %d bytes", count);
    return txBytes;
}

int waitForSerialData(uint8_t nb_uart, int dataCount, int timeout)
{
    int timer = 0;
    int length = 0;
    while (timer < timeout)
    {
        uart_get_buffered_data_len(nb_uart, (size_t *)&length);
        if (length >= dataCount)
        {
            return length;
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
        timer++;
    }
    return 0;
}

void incrementLoadAddress(char *loadAddr)
{
    loadAddr[2] += 0x1;

    if (loadAddr[2] == 0)
    {
        loadAddr[1] += 0x1;

        if (loadAddr[1] == 0)
        {
            loadAddr[0] += 0x1;
        }
    }
}

esp_err_t flashPage(uint8_t nb_uart, const char *address, const char *data)
{
    logI(TAG_STM_PRO, "%s", "Flashing Page");

    cmdWrite(nb_uart);

    loadAddress(nb_uart,address[0], address[1], address[2], address[3]);

    //ESP_LOG_BUFFER_HEXDUMP("FLASH PAGE", data, 256, ESP_LOG_DEBUG);

    char x_or = 0xFF;
    char sz = 0xFF;

    sendData(nb_uart,TAG_STM_PRO, &sz, 1);

    for (int i = 0; i < 256; i++)
    {
        sendData(nb_uart, TAG_STM_PRO, &data[i], 1);
        x_or ^= data[i];
    }

    sendData(nb_uart, TAG_STM_PRO, &x_or, 1);

    int length = waitForSerialData(nb_uart, 1, SERIAL_TIMEOUT);
    if (length > 0)
    {
        uint8_t data[length];
        const int rxBytes = uart_read_bytes(nb_uart, data, length, 1000 / portTICK_RATE_MS);
        if (rxBytes > 0 && data[0] == ACK)
        {
            logI(TAG_STM_PRO, "%s", "Flash Success");
            return ESP_OK;
        }
        else
        {
            logE(TAG_STM_PRO, "%s", "Flash Failure");
            return ESP_FAIL;
        }
    }
    else
    {
        logE(TAG_STM_PRO, "%s", "Serial Timeout");
    }
    return ESP_FAIL;
}

esp_err_t readPage(uint8_t nb_uart, const char *address, const char *data)
{
    logI(TAG_STM_PRO, "%s", "Reading page");
    char param[] = {0xFF, 0x00};

    cmdRead(nb_uart);

    loadAddress(nb_uart,address[0], address[1], address[2], address[3]);

    sendData(nb_uart, TAG_STM_PRO, param, sizeof(param));
    int length = waitForSerialData(nb_uart, 257, SERIAL_TIMEOUT);
    if (length > 0)
    {
        uint8_t uart_data[length];
        const int rxBytes = uart_read_bytes(nb_uart, uart_data, length, 1000 / portTICK_RATE_MS);

        if (rxBytes > 0 && uart_data[0] == 0x79)
        {
            logI(TAG_STM_PRO, "%s", "Success");
            if (!memcpy((void *)data, uart_data, 257))
            {
                return ESP_FAIL;
            }

            //ESP_LOG_BUFFER_HEXDUMP("READ MEMORY", data, rxBytes, ESP_LOG_DEBUG);
        }
        else
        {
            logE(TAG_STM_PRO, "%s", "Failure");
            return ESP_FAIL;
        }
    }
    else
    {
        logE(TAG_STM_PRO, "%s", "Serial Timeout");
        return ESP_FAIL;
    }

    return ESP_OK;
}