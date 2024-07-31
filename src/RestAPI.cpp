#include "RestAPI.h"

RestAPI::RestAPI(txMLRS *txMLRS, int port) : _txMLRS(txMLRS), AsyncWebServer(port)
{
    if (!SPIFFS.begin(true))
    {
        // if you have not used SPIFFS before on a ESP32, it will show this error.
        // after a reboot SPIFFS will be configured and will happily work.
        logE(REST_TAG, "%s", "ERROR: Cannot mount SPIFFS, Rebooting");
        ESP.restart();
    }
    // JSON handler
    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler(
        "/api/v1/cli",
        [this](AsyncWebServerRequest *request, JsonVariant &json)
        { this->jsonCommand(request, json); });
    addHandler(handler);

    // landing page
    on("/", HTTP_GET, [](AsyncWebServerRequest *request)
       {
    logD(REST_TAG, "Client: %s %s", request->client()->remoteIP().toString(), request->url());
    request->send(200, "text/html", index_html, RestAPI::processor); });

    // after firmware upload
    on("/ready", HTTP_GET, [](AsyncWebServerRequest *request)
       {
    logD(REST_TAG, "Client: %s %s", request->client()->remoteIP().toString(), request->url());
    request->send(200, "text/html", upload_html, RestAPI::processor); });

    // Flashing firmware
    on("/flash", HTTP_POST, [this](AsyncWebServerRequest *request)
       {
    logD(REST_TAG, "Client: %s %s", request->client()->remoteIP().toString(), request->url());
    this->flashFirmware();
    request->send(200, "text/html", flash_html, RestAPI::processor); });

    // Flashing firmware
    on("/flash", HTTP_GET, [this](AsyncWebServerRequest *request)
       {
    logD(REST_TAG, "Client: %s %s", request->client()->remoteIP().toString(), request->url());
    request->send(200, "text/html", flash_html, RestAPI::processor); });

    // process upload file
    on("/upload", HTTP_POST, [](AsyncWebServerRequest *request)
       { request->send(200); }, [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
       { this->handleUpload(request, filename, index, data, len, final); });

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

void RestAPI::flashFirmware()
{
    _txMLRS->flash("/firmware.bin");
}

/**
 * @brief
 * source: https://github.com/smford/esp32-asyncwebserver-fileupload-example/blob/master/example-01/example-01.ino
 */
void RestAPI::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index)
    {
        logmessage = "Upload Start: " + String(filename);
        // open the file on first call and store the file handle in the request object
        request->_tempFile = SPIFFS.open("/firmware.bin", "w");
        Serial.println(logmessage);
    }

    if (len)
    {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
        logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        Serial.println(logmessage);
    }

    if (final)
    {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        // close the file handle as the upload is now done
        request->_tempFile.close();
        Serial.println(logmessage);
        request->redirect("/ready");
    }
}

void RestAPI::jsonCommand(AsyncWebServerRequest *request, JsonVariant &json)
{
    if (not json.is<JsonObject>())
    {
        request->send(400, "text/plain", "Not an object");
        return;
    }
    auto &&data = json.as<JsonObject>();

    if (data["write"].is<String>())
    {

        String commandLine = data["write"].as<String>();
        if (commandLine.isEmpty())
        {
            request->send(400, "application/json", "{\"response\":\"no command\"}");
            return;
        }
        if (commandLine == "restart")
        {
            request->send(200, "application/json", "{\"response\":\"restart\"}");
            ESP.restart();
        }
        else
        {
             vTaskSuspendAll();
            _txMLRS->sendCommand(commandLine.c_str());
            xTaskResumeAll();
            request->send(200, "application/json", "{\"response\":\"ok\"}");
        }
    }
    else if (data["read"].is<String>())
    {
        if (_txMLRS->msgReady())
        {
            request->send(200, "application/json", "{\"response\":\"" + String(_txMLRS->response()) + "\"}");
        }
        else {
            request->send(200, "application/json", "{\"response\":\"not ready\"}");
        }
    }
    else
    {
        request->send(400, "application/json", "{\"response\":\"no command\"}");
        return;
    }
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

// Make size of files human readable
// source: https://github.com/CelliesProjects/minimalUploadAuthESP32
String RestAPI::humanReadableSize(const size_t bytes)
{
    if (bytes < 1024)
        return String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        return String(bytes / 1024.0 / 1024.0) + " MB";
    else
        return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

/**
 * @brief
 * source: https://github.com/smford/esp32-asyncwebserver-fileupload-example/blob/master/example-01/example-01.ino
 */
String RestAPI::processor(const String &var)
{
    if (var == "FREESPIFFS")
    {
        return RestAPI::humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
    }

    if (var == "USEDSPIFFS")
    {
        return RestAPI::humanReadableSize(SPIFFS.usedBytes());
    }

    if (var == "TOTALSPIFFS")
    {
        return RestAPI::humanReadableSize(SPIFFS.totalBytes());
    }

    if (var == "FLASHINGSTATUS")
    {
        return String(txMLRS::getFlashStatus());
    }

    return String();
}