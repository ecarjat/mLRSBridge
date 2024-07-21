#ifndef TXMLRS_H
#define TXMLRS_H
#include <Arduino.h>
#define BUF_SIZE 2048

static const char *TXMLRS_TAG = "TXMLRS";


class txMLRS {
    const uint8_t _pin;
    HardwareSerial *_serial;
    int _pos = 0;
    int _maxBuf = BUF_SIZE;
    bool _cli = false;
    bool _msgReady;
    char _buf[BUF_SIZE];
    unsigned long _transmit_ms;
    void flushRX();
    void clear();
    void addc(uint8_t c);

public:
    txMLRS(uint8_t pin, HardwareSerial *serial);
    void begin();
    size_t sendCommand(const char *command);
    void setCli(bool activate);
    bool isActive() { return _cli; }
    bool msgReady() { return _msgReady; }
    String response() {return _msgReady ? String(_buf) : String(""); }
    void loop();
};
#endif