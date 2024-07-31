#include "txMLRS.h"
#include "esp_task_wdt.h"

long txMLRS::_pct_complete = 0.0;

txMLRS::txMLRS(uint8_t pin, uint8_t uart_nb, uint8_t tx, uint8_t rx, IPAddress dest_ip, int udp_port, int baudrate)
    : _pin(pin), _uart_nb(uart_nb), _tx(tx), _rx(rx), _dest_ip(dest_ip), _udp_port(udp_port), _baudrate(baudrate)
{
    switch (uart_nb)
    {
    case 0:
        _serial = &Serial;
        break;
    case 1:
        _serial = &Serial1;
        break;
    case 2:
        _serial = &Serial2;
    }
    udp = new txMAVUDPRelay(_serial, _udp_port);
    _buf[0] = '\0';
}

void txMLRS::begin()
{
    _serial->setRxBufferSize(2 * 1024); // must come before uart started, retuns 0 if it fails
    _serial->setTxBufferSize(512);      // must come before uart started, retuns 0 if it fails
    _serial->begin(_baudrate, SERIAL_8N1, _rx, _tx);
    udp->begin();
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
    if (activate)
    {
        digitalWrite(_pin, LOW);
        _task = CLI;
    }
    else
    {
        _task = BRIDGE;
        digitalWrite(_pin, HIGH);
    }
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

void txMLRS::flash(const char *path)
{
    _firmware_path = path;
    _task = FLASH;
}

/**
 * @brief Activate CLI pin and send command to cli
 *
 * @param command
 */
void txMLRS::sendCommand(const char *command)
{
    strcpy(_buf, command);
    _hasCommand = true;
    _msgReady = false;
    setCli(true);
}

void txMLRS::loop()
{
    switch (_task)
    {
    case CLI:
    {
        if (_hasCommand)
        {
            flushRX();
            logD(TXMLRS_TAG, "about to send: %s length: %d", _buf, strlen(_buf));
            _serial->flush();
            _serial->write(_buf);
            _serial->flush();
            clear();
            _transmit_ms = millis();
            _hasCommand = false;
        }
        int old_pos = _pos;
        while (_serial->available())
        {
            uint8_t c = _serial->read();
            if (c == '\r')
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
        break;
    }
        // }
    case FLASH:
    {
        if (strlen(_firmware_path) == 0)
        {
            logD(TXMLRS_TAG, "%s", "Firmware is null");
            _task = BRIDGE;
            break;
        }
        File flash_file = SPIFFS.open(_firmware_path, "rb");
        if (!flash_file)
        {
            logD(TXMLRS_TAG, "%s", "file does not exist");
            break;
        }
        // put STM32 in systemboot mode
        logI(TXMLRS_TAG, "%s", "STM32 in systemboot mode");
        _serial->flush();
        digitalWrite(_pin, LOW);
        _serial->write("systemboot;\n");
        delay(31);
        _serial->end();
        digitalWrite(_pin, HIGH);
        // pinMode(_pin, INPUT); // check if this can be deleted ?
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

        logD(TXMLRS_TAG, "%s", "flashing firmware");
        if (flash_file.available())
        {
            // This while loop executes only once and breaks if any of the functions do not return ESP_OK
            do
            {
                logI(TXMLRS_TAG, "%s", "Writing STM32 Memory");
                IS_ESP_OK(writeTask(&flash_file));

                logI(TXMLRS_TAG, "%s", "Reading STM32 Memory");
                IS_ESP_OK(readTask(&flash_file));

                logI(TXMLRS_TAG, "%s", "STM32 Flashed Successfully!!!");
            } while (0);
        }

        // re-establish CLI PIN
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, HIGH);
        _serial->setRxBufferSize(2 * 1024); // must come before uart started, retuns 0 if it fails
        _serial->setTxBufferSize(512);      // must come before uart started, retuns 0 if it fails
        _serial->begin(115200, SERIAL_8N1, _rx, _tx);
        _task = BRIDGE;
        break;
    }
    case BRIDGE:
    default:
        udp->loop();
        break;
    }
}

esp_err_t txMLRS::writeTask(File *f)
{
    logI(TAG_STM_FLASH, "%s", "Write Task");

    char loadAddress[4] = {0x08, 0x00, 0x00, 0x00};
    char block[256] = {0};
    int curr_block = 0, bytes_read = 0;

    f->seek(0, fs::SeekSet);
    int nb_blocks = f->size() / 256;
    esp_task_wdt_reset();
    setupSTM(_uart_nb);
    while ((bytes_read = f->read((uint8_t *)block, 256)) > 0)
    {
        curr_block++;
        logI(TAG_STM_FLASH, "Writing block: %d", curr_block);
        // ESP_LOG_BUFFER_HEXDUMP("Block:  ", block, sizeof(block), ESP_LOG_DEBUG);

        esp_err_t ret = flashPage(_uart_nb, loadAddress, block);
        if (ret == ESP_FAIL)
        {
            logE(TAG_STM_FLASH, "Failed flashing page: %x", flashPage);
            return ESP_FAIL;
        }
        setFlashStatus(nb_blocks / curr_block);
        esp_task_wdt_reset();
        incrementLoadAddress(loadAddress);
        printf("\n");

        memset(block, 0xff, 256);
    }

    return ESP_OK;
}

esp_err_t txMLRS::readTask(File *f)
{
    logI(TAG_STM_FLASH, "%s", "Read & Verification Task");
    char readAddress[4] = {0x08, 0x00, 0x00, 0x00};

    char block[257] = {0};
    int curr_block = 0, bytes_read = 0;

    f->seek(0, fs::SeekSet);

    while ((bytes_read = f->read((uint8_t *)block, 256)) > 0)
    {
        curr_block++;
        logI(TAG_STM_FLASH, "Reading block: %d", curr_block);
        // ESP_LOG_BUFFER_HEXDUMP("Block:  ", block, sizeof(block), ESP_LOG_DEBUG);

        esp_err_t ret = readPage(_uart_nb, readAddress, block);
        if (ret == ESP_FAIL)
        {
            return ESP_FAIL;
        }
        esp_task_wdt_reset();
        incrementLoadAddress(readAddress);
        printf("\n");

        memset(block, 0xff, 256);
    }

    return ESP_OK;
}