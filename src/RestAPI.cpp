#include "RestAPI.h"

RestAPI::RestAPI(txMLRS *txMLRS, int port) : _txMLRS(txMLRS), WebServer(port)
{
    setupRouting();
}


void RestAPI::command()
{
    if (hasArg("plain") == false)
    {
        // handle error here
    }
    String body = arg("plain");
    deserializeJson(_jsonDocument, body);
    const char *commandLine = _jsonDocument["command"];
    logD(REST_TAG, "commandline: %s", commandLine);
    if (commandLine == NULL)
        return;
    if (strlen(commandLine) <= 1)
        return;
    if (strcmp("restart", commandLine) == 0)
    {
        send(200, "application/json", "{\"command\":\"ok\"}");
        ESP.restart();
    } else {
        _txMLRS->sendCommand(commandLine);
        unsigned long timeout = millis() + 5000;
        while (!_txMLRS->msgReady() && timeout > millis())
        {
            _txMLRS->loop();
        }
        logD(REST_TAG, "received response: %s", _txMLRS->response());
        send(200, "application/json", "{\"response\":\"" + _txMLRS->response() + "\"}");
    }
}


void RestAPI::data()
{
    _jsonDocument.clear(); // Clear json buffer
    // if (_drybox->hasStarted())
    // {
    //     Sensor *s = _drybox->getHeater()->getSensor();
    //     add_json_object("temperature", s->getTemp(), "°C");
    //     add_json_object("targetTemp", _drybox->getHeater()->getTargetTemp(), "°C");
    //     add_json_object("heatPower", _drybox->getHeater()->getPower() / 255 * 100, "%");
    //     add_json_object("humidity", s->getHumidity(), "%");
    //     add_json_object("fanPower", _drybox->getFan()->getSpeed(), "%");
    //     add_json_object("autotune", _drybox->getHeater()->tuning(), "bool");
    // }
    // else
    // {
    //     add_json_object("started", false, "bool");
    // }
    //serializeJson(_jsonDocument, buffer);
    send(200, "application/json", buffer);
}


void RestAPI::create_json(char *tag, float value, char *unit)
{
    _jsonDocument.clear();
    _jsonDocument["type"] = tag;
    _jsonDocument["value"] = value;
    _jsonDocument["unit"] = unit;
    serializeJson(_jsonDocument, buffer);
}

void RestAPI::add_json_object(const char *tag, float value, const char *unit)
{
    JsonObject obj = _jsonDocument.createNestedObject();
    obj["type"] = tag;
    obj["value"] = value;
    obj["unit"] = unit;
}

void RestAPI::add_json_object(const char *tag, String value, const char *unit)
{
    JsonObject obj = _jsonDocument.createNestedObject();
    obj["type"] = tag;
    obj["value"] = value;
    obj["unit"] = unit;
}

void RestAPI::setCrossOrigin()
{
    sendHeader(F("Access-Control-Max-Age"), F("600"));
    sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void RestAPI::sendCrossOriginHeader()
{
    log_d("sendCORSHeader");
    sendHeader(F("access-control-allow-credentials"), F("false"));
    setCrossOrigin();
    send(204);
}

void RestAPI::setupRouting()
{
    on("/data", [this]()
       { this->data(); });
    on("/command", [this]()
       { this->command(); });
    on("/command", HTTP_OPTIONS, [this]()
       { this->sendCrossOriginHeader(); });
    enableCORS(true);
    begin();
}