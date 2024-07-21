#ifndef RESTAPI_H
#define RESTAPI_H

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AsyncJson.h>
#include "txMLRS.h"
#include "logger.h"

static const char *REST_TAG = "RESTAPI";

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

public:
    RestAPI(txMLRS *txMLRS, int port = 80);
   
};
#endif