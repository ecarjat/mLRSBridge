// ############################################
//  Based off
//  mLRS Wireless Bridge for ESP32
//  Copyright (c) www.olliw.eu, OlliW, OlliW42
//  License: GPL v3
//  https://www.gnu.org/licenses/gpl-3.0.de.html

#include <Arduino.h>
#include <WiFi.h>
#include "logger.h"
#include "txMLRS.h"
#include "RestAPI.h"
#include "UDPServer.h"

#define SERIAL_RXD 16 // = RX2
#define SERIAL_TXD 17 // = TX2
#define CLI_PIN 5
#define UDP_PORT 14550

#ifdef ARDUINO_LOLIN_C3_MINI
#define SERIAL_INTERFACE Serial1
#endif

#ifdef ARDUINO_ESP32_DEV
#define SERIAL_INTERFACE Serial2
#endif



static const char *TAG = "MAIN";
String ssid = "mLRS AP"; // Wifi name
String password = "";    // "thisisgreat"; // WiFi password, "" makes it an open AP



int port_tcp = 5760; // connect to this port per TCP // MissionPlanner default is 5760
// WiFi channel
// 1 is the default, 13 (2461-2483 MHz) has the least overlap with mLRS 2.4 GHz frequencies.
// Note: Channel 13 is generally not available in the US, where 11 is the maximum.
int wifi_channel = 6;

// WiFi power
// comment out for default setting
// Note: In order to find the possible options, right click on WIFI_POWER_19_5dBm and choose "Go To Definiton"
#define WIFI_POWER WIFI_POWER_2dBm // WIFI_POWER_MINUS_1dBm is the lowest possible, WIFI_POWER_19_5dBm is the max

//************************//
//*** General settings ***//

// Baudrate
int baudrate = 115200;

// IPAddress ip_udp(ip[0], ip[1], ip[2], ip[3] + 1); // usually the client/MissionPlanner gets assigned +1

IPAddress ip(192, 168, 4, 55); // connect to this IP // MissionPlanner default is 127.0.0.1, so enter
IPAddress ip_gateway(0, 0, 0, 0);
IPAddress netmask(255, 255, 255, 0);
UDPServer *udp;
txMLRS *tx;
RestAPI *api;

bool led_state;
unsigned long led_tlast_ms;
bool is_connected;
unsigned long is_connected_tlast_ms;
unsigned long serial_data_received_tfirst_ms;

void serialFlushRx(void)
{
  while (SERIAL_INTERFACE.available() > 0)
  {
    SERIAL_INTERFACE.read();
  }
}

// void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
//   IPAddress ip(info.got_ip.ip_info.ip.addr);
//   logD(TAG, "Station connected IP Add = %s", ip.toString());
//   udp->addIP(ip);
// }
 

void setup()
{
  Serial.begin(115200);
  delay(1000);
  esp_log_level_set("*", ESP_LOG_VERBOSE);
  size_t rxbufsize = SERIAL_INTERFACE.setRxBufferSize(2 * 1024); // must come before uart started, retuns 0 if it fails
  size_t txbufsize = SERIAL_INTERFACE.setTxBufferSize(512);      // must come before uart started, retuns 0 if it fails
  // SERIAL_INTERFACE.begin(baudrate);
  SERIAL_INTERFACE.begin(baudrate, SERIAL_8N1, SERIAL_RXD, SERIAL_TXD);
  tx = new txMLRS(CLI_PIN, &SERIAL_INTERFACE);
  udp = new UDPServer(&SERIAL_INTERFACE, UDP_PORT);
  udp->setDestIP(IPAddress(192, 168, 4, 255));

  // WiFi.onEvent(WiFiStationConnected, WiFiEvent_t::ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED);
  WiFi.mode(WIFI_AP); // seems not to be needed, done by WiFi.softAP()?
  WiFi.softAPConfig(ip, ip_gateway, netmask);
  String ssid_full = ssid + " UDP";
  WiFi.softAP(ssid_full.c_str(), (password.length()) ? password.c_str() : NULL, wifi_channel); // channel = 1 is default
  WiFi.softAP(ssid_full.c_str(), (password.length()) ? password.c_str() : NULL, wifi_channel); // channel = 1 is default
  WiFi.setTxPower(WIFI_POWER); // set WiFi power, AP or STA must have been started, returns false if it fails

  logD(TAG, "ap ip address: %s", WiFi.softAPIP().toString());
  
  udp->begin();
  serialFlushRx();
  tx->begin();
  api = new RestAPI(tx, 80);
}

void loop()
{

  // unsigned long tnow_ms = millis();

  // if (is_connected && (tnow_ms - is_connected_tlast_ms > 2000))
  // { // nothing from GCS for 2 secs
  //   is_connected = false;
  // }

  // if (tnow_ms - led_tlast_ms > (is_connected ? 500 : 200))
  // {
  //   led_tlast_ms = tnow_ms;
  //   led_state = !led_state;
  // }
  // uint8_t buf[256]; // working buffer

  // int packetSize = udp.parsePacket();
  // if (packetSize)
  // {
  //   int len = udp.read(buf, sizeof(buf));
  //   SERIAL_INTERFACE.write(buf, len);
  //   is_connected = true;
  //   is_connected_tlast_ms = millis();
  // }

  // tnow_ms = millis(); // may not be relevant, but just update it
  // int avail = SERIAL_INTERFACE.available();
  // if (avail <= 0)
  // {
  //   serial_data_received_tfirst_ms = tnow_ms;
  // }
  // else if ((tnow_ms - serial_data_received_tfirst_ms) > 10 || avail > 128)
  // { // 10 ms at 57600 bps corresponds to 57 bytes, no chance for 128 bytes
  //   serial_data_received_tfirst_ms = tnow_ms;

  //   int len = SERIAL_INTERFACE.read(buf, sizeof(buf));
  //   udp.beginPacket(ip_udp, port_udp);
  //   udp.write(buf, len);
  //   udp.endPacket();
  // }
  if (!tx->isActive())
  {
    udp->loop();
  }
  api->handleClient();
  tx->loop();
}
