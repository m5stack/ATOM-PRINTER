#include "ATOM_PRINTER.h"
#include "ATOM_PRINTER_WEB.h"
#include "ATOM_PRINTER_CONFIG.h"
#include "ATOM_PRINTER_WIFI.h"
#include "ATOM_PRINTER_HTML.h"
#include "ATOM_PRINTER_MQTT.h"
#include <Preferences.h>
#include <ArduinoJson.h>

extern Preferences preferences;
extern ATOM_PRINTER printer;
extern String ssid_html;
extern WebServer webServer;
extern DNSServer dnsServer;
extern DynamicJsonDocument payload;
extern String mqtt_topic;
extern uint8_t bmp_buffer[BMP_BUFFER_LIMIT];
extern int bmp_data_offset;
extern int bmp_data_size;
extern int bmp_width;
extern int bmp_height;

extern PubSubClient mqttClient;
extern xSemaphoreHandle xMQTTMutex;

void handleRoot() {
    webServer.send(200, "text/html", (char*)printer_html);
}

void handleState() {
    DynamicJsonDocument payload(1024);
    payload["WIFI_HTML"] = ssid_html.c_str();
    if (WiFi.status() == WL_CONNECTED) {
        payload["WIFI_STATE"] = true;
        if (mqttClient.connected()) {
            payload["MQTT_STATE"] =
                "Server:" + mqtt_broker + "\r\nSubscribe Topic: " + mqtt_topic;
        } else {
            payload["MQTT_STATE"] = false;
        }
    } else {
        payload["WIFI_STATE"] = false;
        payload["MQTT_STATE"] = false;
    }
    String res;
    serializeJson(payload, res);
    Serial.println(res);
    webServer.send(200, "text/html", res);
}

void handleWiFiConfig() {
    String message;
    for (uint8_t i = 0; i < webServer.args(); i++) {
        message += " NAME:" + webServer.argName(i) +
                   "\n VALUE:" + webServer.arg(i) + "\n";
    }
    Serial.println(message);
    deserializeJson(payload, webServer.arg(0));
    JsonObject obj  = payload.as<JsonObject>();
    String ssid     = obj[String("ssid")];
    String password = obj[String("password")];
    Serial.println("ssid: " + ssid);
    Serial.println("password: " + password);
    if (wifiConnect(ssid, password, 10000)) {
        webServer.send(200, "text/html", "OK");
    } else {
        webServer.send(200, "text/html", "ERROR");
    }
}

void handleMQTTConfig() {
    String message;
    for (uint8_t i = 0; i < webServer.args(); i++) {
        message += " NAME:" + webServer.argName(i) +
                   "\n VALUE:" + webServer.arg(i) + "\n";
    }
    Serial.println(message);
    deserializeJson(payload, webServer.arg(0));
    JsonObject obj         = payload.as<JsonObject>();
    String mqtt_broker     = obj[String("mqtt_broker")];
    int mqtt_port          = obj[String("mqtt_port")];
    String mqtt_id         = obj[String("mqtt_id")];
    String mqtt_user       = obj[String("mqtt_user")];
    String mqtt_password   = obj[String("mqtt_password")];
    String mqtt_topic_info = obj[String("mqtt_topic_info")];

    Serial.println("mqtt_broker: " + mqtt_broker);
    Serial.println("mqtt_port: " + String(mqtt_port));
    Serial.println("mqtt_id: " + mqtt_id);
    Serial.println("mqtt_user: " + mqtt_user);
    Serial.println("mqtt_password: " + mqtt_password);
    Serial.println("mqtt_topic: " + mqtt_topic_info);

    xSemaphoreTake(xMQTTMutex, portMAX_DELAY);
    mqtt_topic = mqtt_topic_info;
    if (mqttConnect(mqtt_broker, mqtt_port, mqtt_id, mqtt_user, mqtt_password,
                    2000)) {
        webServer.send(200, "text/html",
                       "Server:" + mqtt_broker +
                           "\r\nSubscribe Topic: " + mqtt_topic_info);
    } else {
        webServer.send(200, "text/html", "ERROR");
    }
    xSemaphoreGive(xMQTTMutex);
}

void handleBMPSize() {
    String message;
    for (uint8_t i = 0; i < webServer.args(); i++) {
        message += " NAME:" + webServer.argName(i) +
                   "\n VALUE:" + webServer.arg(i) + "\n";
    }
    Serial.println(message);
    deserializeJson(payload, webServer.arg(0));
    JsonObject obj = payload.as<JsonObject>();
    bmp_width      = obj[String("bmp_width")];
    bmp_height     = obj[String("bmp_height")];
    Serial.println("BMP width: " + String(bmp_width));
    Serial.println("BMP height: " + String(bmp_height));
    Serial.println();
    webServer.send(200, "text/html", "OK");
}

void handleBMP() {
    HTTPUpload& upload = webServer.upload();
    if (upload.status == UPLOAD_FILE_START) {
        String filename = upload.filename;
        Serial.print("handleFileUpload Name: ");
        Serial.println(filename);
        bmp_data_offset = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        Serial.print("handleFileUpload Size: ");
        Serial.println(upload.currentSize);
        size_t size = upload.currentSize;
        memcpy(bmp_buffer + bmp_data_offset, upload.buf, size);
        bmp_data_offset += size;
    } else if (upload.status == UPLOAD_FILE_END) {
        Serial.print("handleFileUpload totalSize: ");
        Serial.println(upload.totalSize);
        bmp_data_size = upload.totalSize;
        printer.printBMP(0, bmp_width, bmp_height, bmp_buffer);
    }
}

void handlePrint() {
    String message;
    for (uint8_t i = 0; i < webServer.args(); i++) {
        message += " NAME:" + webServer.argName(i) +
                   "\n VALUE:" + webServer.arg(i) + "\n";
    }
    Serial.println(message);
    deserializeJson(payload, webServer.arg(0));
    JsonObject obj = payload.as<JsonObject>();
    String type    = obj[String("type")];
    String payload = obj[String("payload")];
    Serial.println("type: " + type);
    Serial.println("payload: " + payload);
    printer.init();
    if (type == "QRCODE") {
        printer.init();
        printer.printQRCode(payload);
        printer.newLine(3);
    } else if (type == "BAR") {
        printer.init();
        printer.setBarCodeHRI(ABOVE);
        printer.printBarCode(UPC_A, payload);
    } else {
        printer.init();
        printer.printASCII(payload);
        printer.newLine(3);
    }
    webServer.send(200, "text/html", "OK");
}

void setWebServer() {
    dnsServer.start(
        DNS_PORT, "*",
        apIP);  // 强制门户认证（需要设置notfound时候的网页，否则不会弹出）
    webServer.onNotFound(handleRoot);
    webServer.on("/", handleRoot);
    webServer.on("/state", handleState);
    webServer.on("/print", HTTP_POST, handlePrint);
    webServer.on("/config", HTTP_POST, handleWiFiConfig);
    webServer.on("/mqtt", HTTP_POST, handleMQTTConfig);
    webServer.on("/bmp_size", HTTP_POST, handleBMPSize);
    //   webServer.on("/bmp", HTTP_POST, handleBMP);
    webServer.on(
        "/bmp", HTTP_POST, []() { webServer.send(200, "text/plain", "OK"); },
        handleBMP);

    webServer.begin();
    Serial.println("HTTP server started");
}
