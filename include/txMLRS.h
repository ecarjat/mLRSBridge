/*
MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef TXMLRS_H
#define TXMLRS_H
#include <Arduino.h>
#include <SPIFFS.h>
#include "logger.h"
#include "driver/uart.h"
#include "stm_pro_mode.h"
#include "txMAVUDPRelay.h"
#define BUF_SIZE 2048
#define UART_BUF_SIZE 1024
#define PATH_SIZE 20

static const char *TXMLRS_TAG = "TXMLRS";
static const char *TAG_STM_FLASH = "stm_flash";

class txMLRS
{
    friend class STMFlash;
    const uint8_t _pin;
    HardwareSerial *_serial;
    const uint8_t _uart_nb;
    const int _udp_port;
    const uint8_t _tx;
    const uint8_t _rx;
    const int _baudrate;
    IPAddress _dest_ip;
    enum taskList
    {
        CLI,
        FLASH,
        BRIDGE
    };
    txMAVUDPRelay *udp;
    bool _hasCommand = false;
    int _pos = 0;
    int _maxBuf = BUF_SIZE;
    // bool _cli = false;
    bool _msgReady;
    char _buf[BUF_SIZE];
    unsigned long _transmit_ms;
    const char *_firmware_path;
    taskList _task;
    static long _pct_complete;
    void flushRX();
    void clear();
    void addc(uint8_t c);

public:
    txMLRS(uint8_t pin, uint8_t uart_nb, uint8_t tx, uint8_t rx, IPAddress dest_ip, int udp_port, int baudrate = 115200);
    void begin();
    void sendCommand(const char *command);
    void setCli(bool activate);
    // bool isActive() { return _task == CLI; }
    bool msgReady() { return _msgReady; }
    const char *response() { return _msgReady ? _buf : ""; }
    void loop();
    void flash(const char *path);
    static void setFlashStatus(long percent)
    {
        _pct_complete = percent;
    }
    static long getFlashStatus()
    {
        return _pct_complete;
    }

    /*
    Imported from
    https://github.com/ESP32-Musings/OTA_update_STM32_using_ESP32
    Copyright (c) 2020 Laukik Hase
    */
    /**
     * @brief Write the code into the flash memory of STM32Fxx
     *
     * The data from the .bin file is written into the flash memory
     * of the client, block-by-block
     *
     * @param flash_file File pointer of the .bin file to be flashed
     *
     * @return ESP_OK - success, ESP_FAIL - failed
     */
    esp_err_t writeTask(File *f);

    /**
     * @brief Read the flash memory of the STM32Fxx, for verification
     *
     * It reads the flash memory of the STM32 block-by-block and
     * checks it with the data from the file (with pointer passed)
     *
     * @param flash_file File pointer of the .bin file to be verified against
     *
     * @return ESP_OK - success, ESP_FAIL - failed
     */
    esp_err_t readTask(File *f);
};
#endif