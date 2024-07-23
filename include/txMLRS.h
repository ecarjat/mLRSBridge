#ifndef TXMLRS_H
#define TXMLRS_H
#include <Arduino.h>
#include <SPIFFS.h>
#include "logger.h"
#include "driver/uart.h"
#include "stm_flash.h"
#define BUF_SIZE 2048
#define UART_BUF_SIZE 1024

static const char *TXMLRS_TAG = "TXMLRS";


class txMLRS {
    const uint8_t _pin;
    HardwareSerial *_serial;
    const uint8_t _uart_nb;
    const uint8_t _tx;
    const uint8_t _rx;
    int _pos = 0;
    int _maxBuf = BUF_SIZE;
    bool _cli = false;
    bool _msgReady;
    char _buf[BUF_SIZE];
    unsigned long _transmit_ms;
    void flushRX();
    void clear();
    void addc(uint8_t c);
    int waitForSerialData(int timeout);

public:
    txMLRS(uint8_t pin, HardwareSerial *serial, uint8_t uart_nb, uint8_t tx, uint8_t rx);
    void begin();
    size_t sendCommand(const char *command);
    void setCli(bool activate);
    bool isActive() { return _cli; }
    bool msgReady() { return _msgReady; }
    String response() {return _msgReady ? String(_buf) : String(""); }
    void loop();
    void flash(File *f);
};
#endif