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
String Pdata, newLine, QRCode, AdjustLevel, printType, BarCode, BarType, Position;

String urlDecode(String input)
{
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

void handleRoot()
{
    webServer.send(200, "text/html", (char*)printer_html);
}

void handleWiFiConfig()
{
    Serial.println("Handling WiFi config request");

    // 检查是否有请求体
    if (!webServer.hasArg("plain")) {
        webServer.send(400, "text/plain", "Bad Request: No JSON data");
        return;
    }

    // 解析JSON数据
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        webServer.send(400, "text/plain", "Bad Request: Invalid JSON");
        return;
    }

    String ssid     = doc["ssid"].as<String>();
    String password = doc["password"].as<String>();

    Serial.println("Received WiFi config:");
    Serial.print("SSID: ");
    Serial.println(ssid);
    Serial.print("Password: ");
    Serial.println(password);

    // 验证SSID
    if (ssid.length() == 0) {
        webServer.send(400, "text/plain", "Error: SSID cannot be empty");
        return;
    }

    // 尝试连接WiFi
    bool connectSuccess = wifiConnect(ssid, password, 5000);

    if (connectSuccess) {
        webServer.send(200, "text/plain", "OK");
        Serial.println("WiFi connected successfully");
    } else {
        webServer.send(500, "text/plain", "Error: Failed to connect to WiFi");
        Serial.println("WiFi connection failed");
    }
}

void handleStatusConfig()
{
    Serial.println("Handling status request");

    DynamicJsonDocument doc(1024);

    // WiFi状态
    if (WiFi.status() == WL_CONNECTED) {
        doc["WIFI_STATE"] = true;
        doc["SSID"]       = WiFi.SSID();
        doc["IP"]         = WiFi.localIP().toString();
        doc["RSSI"]       = WiFi.RSSI();

        // MQTT状态
        xSemaphoreTake(xMQTTMutex, portMAX_DELAY);
        if (mqttClient.connected()) {
            doc["MQTT_STATE"]  = "Connected";
            doc["MQTT_BROKER"] = mqtt_broker;
            doc["MQTT_TOPIC"]  = mqtt_topic;
        } else {
            doc["MQTT_STATE"] = "Disconnected";
        }
        xSemaphoreGive(xMQTTMutex);
    } else {
        doc["WIFI_STATE"] = false;
        doc["MQTT_STATE"] = "Disconnected";
    }

    // 可用的WiFi网络列表
    doc["WIFI_HTML"] = ssid_html;

    // 发送响应
    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);

    Serial.println("Status response sent");
}

void handleMQTTConfig()
{
    Serial.println("Handling MQTT config request");

    // 检查是否有请求体
    if (!webServer.hasArg("plain")) {
        webServer.send(400, "text/plain", "Bad Request: No JSON data");
        return;
    }

    // 解析JSON数据
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        webServer.send(400, "text/plain", "Bad Request: Invalid JSON");
        return;
    }

    String broker   = doc["mqtt_broker"].as<String>();
    int port        = doc["mqtt_port"].as<int>();
    String id       = doc["mqtt_id"].as<String>();
    String user     = doc["mqtt_user"].as<String>();
    String password = doc["mqtt_password"].as<String>();
    String topic    = doc["mqtt_topic_info"].as<String>();

    Serial.println("Received MQTT config:");
    Serial.print("Broker: ");
    Serial.println(broker);
    Serial.print("Port: ");
    Serial.println(port);
    Serial.print("Client ID: ");
    Serial.println(id);
    Serial.print("User: ");
    Serial.println(user);
    Serial.print("Topic: ");
    Serial.println(topic);

    // 验证输入
    if (broker.length() == 0 || port <= 0 || port > 65535) {
        webServer.send(400, "text/plain", "Error: Invalid broker or port");
        return;
    }

    xSemaphoreTake(xMQTTMutex, portMAX_DELAY);
    mqtt_topic          = topic;
    bool connectSuccess = mqttConnect(broker, port, id, user, password, 2000);
    xSemaphoreGive(xMQTTMutex);

    if (connectSuccess) {
        String response = "Server:" + broker + "\nPort:" + String(port) + "\nTopic:" + topic;
        webServer.send(200, "text/plain", response);
        Serial.println("MQTT connected successfully");
    } else {
        webServer.send(500, "text/plain", "Error: Failed to connect to MQTT broker");
        Serial.println("MQTT connection failed");
    }
}

void handleBMPSize()
{
    Serial.println("Handling BMP size request");

    // 检查是否有请求体
    if (!webServer.hasArg("plain")) {
        webServer.send(400, "text/plain", "Bad Request: No JSON data");
        return;
    }

    // 解析JSON数据
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, webServer.arg("plain"));

    if (error) {
        Serial.print("JSON parse failed: ");
        Serial.println(error.c_str());
        webServer.send(400, "text/plain", "Bad Request: Invalid JSON");
        return;
    }

    bmp_width  = doc["bmp_width"].as<int>();
    bmp_height = doc["bmp_height"].as<int>();

    Serial.print("BMP width: ");
    Serial.println(bmp_width);
    Serial.print("BMP height: ");
    Serial.println(bmp_height);

    webServer.send(200, "text/plain", "OK");
}

void handleBMP()
{
    HTTPUpload& upload = webServer.upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.print("BMP upload started. Filename: ");
        Serial.println(upload.filename);
        bmp_data_offset = 0;
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        size_t chunkSize = upload.currentSize;
        if (bmp_data_offset + chunkSize > BMP_BUFFER_LIMIT) {
            Serial.println("Error: BMP file too large");
            return;
        }
        memcpy(bmp_buffer + bmp_data_offset, upload.buf, chunkSize);
        bmp_data_offset += chunkSize;
        Serial.print("Received BMP chunk. Total size: ");
        Serial.println(bmp_data_offset);
    } else if (upload.status == UPLOAD_FILE_END) {
        bmp_data_size = upload.totalSize;
        Serial.print("BMP upload completed. Total size: ");
        Serial.println(bmp_data_size);

        if (bmp_width > 0 && bmp_height > 0) {
            printer.printBMP(0, bmp_width, bmp_height, bmp_buffer);
            webServer.send(200, "text/plain", "BMP printed successfully");
        } else {
            webServer.send(400, "text/plain", "Error: BMP dimensions not set");
        }
    }
}

void handlePrint()
{
    Serial.println("Handling print request");

    // 获取打印类型
    printType = urlDecode(webServer.arg("printType"));
    Serial.print("Print type: ");
    Serial.println(printType);

    // 根据打印类型处理不同的数据
    if (printType == "ASCII") {
        Pdata   = urlDecode(webServer.arg("Pdata"));
        newLine = urlDecode(webServer.arg("newLine"));

        Serial.print("ASCII data: ");
        Serial.println(Pdata);
        Serial.print("New line: ");
        Serial.println(newLine);

        printer.init();
        printer.printASCII(Pdata);
    } else if (printType == "QRCode") {
        QRCode  = urlDecode(webServer.arg("QRCode"));
        newLine = urlDecode(webServer.arg("newLine"));

        Serial.print("QRCode data: ");
        Serial.println(QRCode);

        printer.init();
        printer.printQRCode(QRCode);
    } else if (printType == "BarCode") {
        BarCode = urlDecode(webServer.arg("BarCode"));
        newLine = urlDecode(webServer.arg("newLine"));

        Serial.print("Barcode data: ");
        Serial.println(BarCode);

        printer.init();
        printer.setBarCodeHRI(HIDE);
        printer.printBarCode(CODE128, BarCode);
    } else {
        webServer.send(400, "text/plain", "Error: Invalid print type");
        return;
    }

    // 处理换行
    if (newLine == "on") {
        printer.newLine(1);
    }

    webServer.send(200, "text/plain", "OK");
    Serial.println("Print request completed");
}

void webServerInit()
{
    // 启动DNS服务器
    dnsServer.start(DNS_PORT, "*", apIP);

    // 设置路由处理器
    webServer.onNotFound(handleRoot);
    webServer.on("/", HTTP_GET, handleRoot);
    webServer.on("/print", HTTP_GET, handlePrint);
    webServer.on("/wifi_config", HTTP_POST, handleWiFiConfig);
    webServer.on("/mqtt_config", HTTP_GET, handleMQTTConfig);
    webServer.on("/device_status", HTTP_GET, handleStatusConfig);
    webServer.on("/bmp_size", HTTP_POST, handleBMPSize);
    webServer.on("/bmp", HTTP_POST, []() { webServer.send(200, "text/plain", "Ready for BMP upload"); }, handleBMP);

    // 启动Web服务器
    webServer.begin();

    Serial.println("HTTP server started");
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
}