#ifndef UDPSERVER_H
#define UDPSERVER_H
#include <Arduino.h>
#include <WiFi.h>
#include "logger.h"

static const char *UDPSERVER_TAG = "UDPSERVER";

class UDPServer : public WiFiUDP {

    int _port;
    HardwareSerial *_serial;
    IPAddress _dest_ip;
    uint8_t _nbIPs = 0;
    unsigned long _serial_data_received_tfirst_ms;


public:
    UDPServer(HardwareSerial *serial, int port) : _serial(serial),_port(port) {}
    void setDestIP(IPAddress ip);
    // IPAddress *getIPs(){return _gcs_ips;}
    // uint8_t getNbIPs() { return _nbIPs; }
    void begin() { WiFiUDP::begin(_port); }
    void loop();
};
#endif