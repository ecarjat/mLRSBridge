#include "RestAPI.h"

void RestAPI::jsonCommand(AsyncWebServerRequest *request, JsonVariant &json)
{
    if (not json.is<JsonObject>())
    {
        request->send(400, "text/plain", "Not an object");
        return;
    }
    auto &&data = json.as<JsonObject>();

    if (not data["command"].is<String>())
    {
       request->send(400, "application/json", "{\"response\":\"no command\"}");
        return;
    }
    String commandLine = data["command"].as<String>();
    if (commandLine.isEmpty()){
        request->send(400, "application/json", "{\"response\":\"no command\"}");
        return;
    }
    if (commandLine == "restart")
    {
        request->send(200, "application/json", "{\"command\":\"ok\"}");
        ESP.restart();
    }
    else
    {
        _txMLRS->sendCommand(commandLine.c_str());
        unsigned long timeout = millis() + 3000;
        while (!_txMLRS->msgReady() && timeout > millis())
        {
            _txMLRS->loop();
        }
        request->send(200, "application/json", "{\"response\":\"" + _txMLRS->response() + "\"}");
    }
}

RestAPI::RestAPI(txMLRS *txMLRS, int port) : _txMLRS(txMLRS), AsyncWebServer(port)
{
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
        "/command",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        { this->jsonCommand(request, json); });
    addHandler(handler);

    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    onNotFound([](AsyncWebServerRequest *request)
               {
        if (request->method() == HTTP_OPTIONS) {
            request->send(200);
        } else {
            request->send(404);
    } });
    begin();
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
