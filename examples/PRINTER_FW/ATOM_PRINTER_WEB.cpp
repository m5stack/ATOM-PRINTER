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

// 保存网页获取的数据
String Pdata, newLine, QRCode, AdjustLevel, printType, BarCode, BarType,
    Position;

String urlDecode(String input) {
    String s = input;
    s.replace("%20", " ");
    s.replace("+", " ");
    s.replace("%21", "!");
    s.replace("%22", "\"");
    s.replace("%23", "#");
    s.replace("%24", "$");
    s.replace("%25", "%");
    s.replace("%26", "&");
    s.replace("%27", "\'");
    s.replace("%28", "(");
    s.replace("%29", ")");
    s.replace("%30", "*");
    s.replace("%31", "+");
    s.replace("%2C", ",");
    s.replace("%2E", ".");
    s.replace("%2F", "/");
    s.replace("%2C", ",");
    s.replace("%3A", ":");
    s.replace("%3A", ";");
    s.replace("%3C", "<");
    s.replace("%3D", "=");
    s.replace("%3E", ">");
    s.replace("%3F", "?");
    s.replace("%40", "@");
    s.replace("%5B", "[");
    s.replace("%5C", "\\");
    s.replace("%5D", "]");
    s.replace("%5E", "^");
    s.replace("%5F", "-");
    s.replace("%60", "`");
    return s;
}

void handleRoot() {
    webServer.send(200, "text/html", (char*)printer_html);
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
    if (wifiConnect(ssid, password, 6000)) {
        webServer.send(200, "text/html", "OK");
    } else {
        webServer.send(200, "text/html", "ERROR");
    }
}

void handleStatusConfig() {
    DynamicJsonDocument payload(1024);
    payload["WIFI_HTML"] = ssid_html.c_str();
    if (WiFi.status() == WL_CONNECTED) {
        payload["WIFI_STATE"] = true;
        payload["SSID"]       = wifi_ssid;
        payload["PASSWORD"]   = wifi_password;
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
    webServer.send(200, "text/html", res);
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
    Pdata     = urlDecode(webServer.arg("Pdata"));
    newLine   = urlDecode(webServer.arg("newLine"));
    QRCode    = urlDecode(webServer.arg("QRCode"));
    printType = urlDecode(webServer.arg("printType"));
    // AdjustLevel = urlDecode(webServer.arg("AdjustLevel"));
    // BarType     = urlDecode(webServer.arg("BarType"));
    BarCode = urlDecode(webServer.arg("BarCode"));
    // Position    = urlDecode(webServer.arg("Position"));
    if (printType == "ASCII") {
        printer.init();
        printer.printASCII(Pdata);
        Serial.print(Pdata);
    } else if (printType == "QRCode") {
        printer.init();
        printer.printQRCode(QRCode);
        Serial.print(QRCode);
    } else if (printType == "BarCode") {
        printer.init();
        printer.setBarCodeHRI(HIDE);
        printer.printBarCode(CODE128, BarCode);
        Serial.print(BarCode);
    }
    if (newLine == "on") {
        printer.newLine(1);
    }
    webServer.send(200, "text/html", "OK");
}

void webServerInit() {
    dnsServer.start(
        DNS_PORT, "*",
        apIP);  // 强制门户认证（需要设置notfound时候的网页，否则不会弹出）
    webServer.onNotFound(handleRoot);
    webServer.on("/", handleRoot);
    webServer.on("/print", HTTP_GET, handlePrint);
    webServer.on("/wifi_config", HTTP_POST, handleWiFiConfig);
    webServer.on("/mqtt_config", HTTP_GET, handleMQTTConfig);
    webServer.on("/device_status", HTTP_GET, handleStatusConfig);
    webServer.on("/bmp_size", HTTP_GET, handleBMPSize);
    webServer.on(
         "/bmp", HTTP_POST, []() { webServer.send(200, "text/plain", "OK"); },
         handleBMP);
    webServer.begin();
    Serial.println("HTTP server started");
    Serial.println(
        WiFi.softAPIP());  // IP address assigned to your ESP  获取ip地址
}
