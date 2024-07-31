#ifndef RESTAPI_H
#define RESTAPI_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include <SPIFFS.h>
#include "txMLRS.h"
#include "logger.h"

static const char *REST_TAG = "RESTAPI";

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>File Upload</h1></p>
  <p>Free Storage: %FREESPIFFS% | Used Storage: %USEDSPIFFS% | Total Storage: %TOTALSPIFFS%</p>
  <form method="POST" action="/upload" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload" title="Upload File"></form>
  <p>After clicking upload it will take some time for the file to firstly upload and then be written to SPIFFS, there is no indicator that the upload began.  Please be patient.</p>
  <p>Once uploaded the page will refresh and the newly uploaded file will appear in the file list.</p>
  <p>If a file does not appear, it will be because the file was too big, or had unusual characters in the file name (like spaces).</p>
  <p>You can see the progress of the upload by watching the serial output.</p>
</body>
</html>
)rawliteral";

static const char upload_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>Firmware uploaded, ready to flash</h1></p>
  <p>Free Storage: %FREESPIFFS% | Used Storage: %USEDSPIFFS% | Total Storage: %TOTALSPIFFS%</p>
  <form method="POST" action="/flash"><input type="submit" name="Flash" value="Flash" title="Flash"></form>
</body>
</html>
)rawliteral";

static const char flash_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
  <meta http-equiv="refresh" content="5">
</head>
<body>
  <p><h1>%FLASHINGSTATUS%</h1></p>
  <p>Free Storage: %FREESPIFFS% | Used Storage: %USEDSPIFFS% | Total Storage: %TOTALSPIFFS%</p>
</body>
</html>
)rawliteral";

#define JSONSIZE 2048

class RestAPI : public AsyncWebServer
{
  txMLRS *_txMLRS;
  void jsonCommand(AsyncWebServerRequest *request, JsonVariant &json);
  StaticJsonDocument<JSONSIZE> _jsonDocument;
  char buffer[JSONSIZE];
  void create_json(char *tag, float value, char *unit);
  void add_json_object(const char *tag, float value, const char *unit);
  void add_json_object(const char *tag, String value, const char *unit);
  void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
  void flashFirmware();

public:
  RestAPI(txMLRS *txMLRS, int port = 80);
  static String humanReadableSize(const size_t bytes);
  static String processor(const String &var);
};
#endif