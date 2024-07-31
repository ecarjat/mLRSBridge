#include "txMAVUDPRelay.h"

void txMAVUDPRelay::setDestIP(IPAddress ip)
{
    _dest_ip = ip;
}

void txMAVUDPRelay::loop()
{
    unsigned long tnow_ms = millis();

    uint8_t buf[256]; // working buffer

    int packetSize = parsePacket();
    if (packetSize)
    {
        int len = read(buf, sizeof(buf));
        _serial->write(buf, len);
    }

    tnow_ms = millis(); // may not be relevant, but just update it
    int avail = _serial->available();
    if (avail <= 0)
    {
        _serial_data_received_tfirst_ms = tnow_ms;
    }
    else if ((tnow_ms - _serial_data_received_tfirst_ms) > 10 || avail > 128)
    { // 10 ms at 57600 bps corresponds to 57 bytes, no chance for 128 bytes
        _serial_data_received_tfirst_ms = tnow_ms;
        int len = _serial->read(buf, sizeof(buf));
        beginPacket(_dest_ip, _port);
        write(buf, len);
        endPacket();
    }
}