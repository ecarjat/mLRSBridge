#include "txMLRS.h"
#include "esp_task_wdt.h"

txMLRS::txMLRS(uint8_t pin, HardwareSerial *serial, uint8_t uart_nb, uint8_t tx, uint8_t rx) : _pin(pin), _serial(serial), _uart_nb(uart_nb), _tx(tx), _rx(rx) {}

void txMLRS::begin()
{
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, HIGH);
}

void txMLRS::flushRX()
{
    while (_serial->available() > 0)
    {
        _serial->read();
    }
}

void txMLRS::setCli(bool activate)
{
    _cli = activate;
    activate ? digitalWrite(_pin, LOW) : digitalWrite(_pin, HIGH);
}

void txMLRS::clear()
{
    _pos = 0;
    _buf[_pos] = '\0';
}

void txMLRS::addc(uint8_t c)
{
    if (_pos >= _maxBuf)
        return;
    _buf[_pos++] = c;
    _buf[_pos] = '\0';
}

int txMLRS::waitForSerialData(int timeout)
{
    int timer = 0;
    int length = 0;
    while (timer < timeout)
    {
        if (_serial->available())
        {
            return _serial->read();
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
        timer++;
    }
    return -1;
}

void txMLRS::flash(File *f)
{
    esp_err_t err = ESP_FAIL;
    // pass STM32 in systemboot mode
    logI(TXMLRS_TAG, "%s", "STM32 in systemboot mode");
    int nbchar = sendCommand("systemboot;\n");
    delay(27);

    //digitalWrite(_pin, HIGH);
    // pinMode(_pin, INPUT);
    // logI(TXMLRS_TAG, "written %d chars", nbchar);
    // esp_task_wdt_reset();
    _serial->end();
    // _serial->setRxBufferSize(2 * 1024);
    // _serial->setTxBufferSize(0);
    // _serial->setHwFlowCtrlMode(UART_HW_FLOWCTRL_DISABLE, 64);
    // _serial->begin(115200, SERIAL_8E1, _rx, _tx);

    // int c = -1;
    // while (c != 0x79)
    // {
    //     _serial->write(0x7F);
    //     esp_task_wdt_reset();
    //     c = waitForSerialData(4000);
    //     logI(TXMLRS_TAG, "received char: %x", c);
    // }
    // _serial->end();
    // change uart parameters to firmware upload

    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_EVEN,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
    uart_driver_install(_uart_nb, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(_uart_nb, &uart_config);
    uart_set_pin(_uart_nb, _tx, _rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    logI(TXMLRS_TAG, "%s", "Initialized Flash UART");
    esp_task_wdt_reset();

    // open bin file
    if (f->available())
    {
        // This while loop executes only once and breaks if any of the functions do not return ESP_OK
        do
        {
            logI(TXMLRS_TAG, "%s", "Writing STM32 Memory");
            IS_ESP_OK(writeTask(_uart_nb, f));

            logI(TXMLRS_TAG, "%s", "Reading STM32 Memory");
            IS_ESP_OK(readTask(_uart_nb, f));

            err = ESP_OK;
            logI(TXMLRS_TAG, "%s", "STM32 Flashed Successfully!!!");
        } while (0);
    }

    // re-establish CLI
    pinMode(_pin, OUTPUT);
    setCli(false);
    _serial->setRxBufferSize(2 * 1024); // must come before uart started, retuns 0 if it fails
    _serial->setTxBufferSize(512);      // must come before uart started, retuns 0 if it fails
    _serial->begin(115200, SERIAL_8N1, _rx, _tx);
}

/**
 * @brief Activate CLI pin and send command to cli
 *
 * @param command
 * @return number of chars written
 */
size_t txMLRS::sendCommand(const char *command)
{
    setCli(true);
    flushRX();
    delay(500);
    logD(TXMLRS_TAG, "about to send: %s length: %d", command, strlen(command));
    // clear receive buffer
    clear();
    _msgReady = false;
    // set transmit time
    _transmit_ms = millis();
    _serial->flush();
    size_t nb = _serial->write(command);
    _serial->flush();
    return nb;
}

void txMLRS::loop()
{
    if (_cli)
    {
        int old_pos = _pos;
        while (_serial->available())
        {
            uint8_t c = _serial->read();
            if(c=='\r')
                c = '\n';
            addc(c);
            _transmit_ms = millis();
        }
        // if I haven't received new char for the last second
        if (millis() > _transmit_ms + 1500 && old_pos == _pos)
        {
            setCli(false);
            if (_pos > 0)
            {
                _msgReady = true;
            }
            logD(TXMLRS_TAG, "received chars: %d", _pos);
        }
    }
}