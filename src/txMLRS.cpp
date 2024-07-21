#include "txMLRS.h"
#include "logger.h"

txMLRS::txMLRS(uint8_t pin, HardwareSerial *serial) : _pin(pin), _serial(serial) {}

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

/**
 * @brief Activate CLI pin and send command to cli
 *
 * @param command
 * @return number of chars written
 */
size_t txMLRS::sendCommand(const char *command)
{
    String resp;
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