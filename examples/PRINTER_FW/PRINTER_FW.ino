/*
 * SPDX-FileCopyrightText: 2025 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

/**
 * Please install the following dependent libraries before compiling:
 * M5Atom: https://github.com/m5stack/M5Atom
 * FastLED: https://github.com/FastLED/FastLED
 * PubSubClient: https://github.com/knolleary/pubsubclient
 * ArduinoJson: https://github.com/bblanchon/ArduinoJson
 * @Hardwares: Atom Printer
 * @Platform Version: Arduino M5Stack Board Manager v2.1.4
 */

/*
  How to use:
  1. connect to AP `ATOM_PRINTER-xxxx`
  2. Visit 192.168.4.1 to print
  3. Configure WiFi connection and print data through mqtt server (refer README)
*/

#include <M5Atom.h>
#include "ATOM_PRINTER.h"
#include "ATOM_PRINTER_CONFIG.h"
#include "ATOM_PRINTER_WEB.h"
#include "ATOM_PRINTER_MQTT.h"
#include "ATOM_PRINTER_WIFI.h"
#include <Preferences.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

xSemaphoreHandle xMQTTMutex = xSemaphoreCreateMutex();

Preferences preferences;
ATOM_PRINTER printer;
DynamicJsonDocument payload(1024);
WebServer webServer(80);
DNSServer dnsServer;
const IPAddress apIP(192, 168, 4, 1);

uint8_t bmp_buffer[BMP_BUFFER_LIMIT] = {0};
int bmp_data_offset                  = 0;
int bmp_data_size                    = 0;
int bmp_width                        = 0;
int bmp_height                       = 0;

// wifi设置
const char *apSSID   = "ATOM_PRINTER";
String wifi_ssid     = "";
String wifi_password = "";
String ssid_html;

bool is_config_mode = true;

String device_mac;

// mqtt
String mqtt_broker   = MQTT_BROKER;
int mqtt_port        = MQTT_PORT;
String mqtt_id       = MQTT_ID;
String mqtt_user     = MQTT_USER;
String mqtt_password = MQTT_PASSWORD;
String mqtt_topic    = MQTT_TOPIC;

bool mqtt_connect_change_event = false;

WiFiClient client;
PubSubClient mqttClient(client);

Atom_Printer_State_t device_state = kInit;

void flashing(uint32_t color, uint8_t frequency)
{
    static uint32_t prev_ms = millis();
    static bool rgbState    = 0;
    if (millis() > prev_ms + frequency) {
        prev_ms  = millis();
        rgbState = !rgbState;
    }
    M5.dis.drawpix(0, color * rgbState);
}

void TaskLED(void *pvParameters)
{
    while (1) {
        switch (device_state) {
            case kInit:
                // M5.dis.drawpix(0, 0xffe500);  //yellow
                flashing(0x00ff00, 20);  // blinking green
                break;
            case kWiFiConnected:
                M5.dis.drawpix(0, 0x00ff00);  // green
                break;
            case kWiFiDisconnected:
                flashing(0xff0000, 20);  // blinking red
                break;
            case kMQTTConnected:
                M5.dis.drawpix(0, 0x0000ff);  // blue
                break;
            case kMQTTDisconnected:
                flashing(0x0000ff, 20);  // blinking blue
                break;
        }
        vTaskDelay(500);
    }
}

void mqttCallback(char *topic, byte *payload, unsigned int len)
{
    // mqtt回调函数：将从订阅主题获得的信息通过串口打印
    char PayloadData[len + 1];
    String Type = "";
    int posx;
    uint8_t indexs;
    uint8_t fonts;
    strncpy(PayloadData, (char *)payload, len);
    PayloadData[len] = '\0';
    Serial.println(mqtt_topic + ":");
    Serial.printf("leng:%d\r\n", len);
    Serial.println(String(PayloadData));
    Type = String(PayloadData);
    if (Type.indexOf("TEXT") >= 0) {
        Type   = Type.substring(5);
        posx   = Type.toInt();
        indexs = Type.indexOf(",");
        Type   = Type.substring(indexs + 1);
        fonts  = Type.toInt();
        indexs = Type.indexOf(":");
        printer.init();
        printer.printPos(posx);
        printer.fontSize(fonts);
        printer.printASCII(&Type[indexs + 1]);
        printer.newLine(3);
    } else if (Type.indexOf("QR:") >= 0) {
        printer.init();
        printer.printQRCode(&Type[3]);
        printer.newLine(3);
    } else if (Type.indexOf("BAR:") >= 0) {
        printer.init();
        printer.setBarCodeHRI(HIDE);
        printer.printBarCode(CODE128, &Type[4]);
        printer.newLine(3);
    }
}

void setup()
{
    M5.begin(true, false, true);
    printer.begin();
    M5.dis.drawpix(0, 0x00ffff);  // 初始化状态灯
    preferences.begin("PRINTER_CONFIG");
    // disableCore0WDT();
    printer.init();
    // printer.newLine(1);
    // Create LED Task
    xTaskCreatePinnedToCore(TaskLED, "TaskLED"  // A name just for humans
                            ,
                            2048  // This stack size can be checked & adjusted
                                  // by reading the Stack Highwater
                            ,
                            NULL,
                            3  // Priority, with 3 (configMAX_PRIORITIES - 1)
                               // being the highest, and 0 being the lowest.
                            ,
                            NULL, 0);

    wifiInit();
    ssid_html = wifiScan();
    webServerInit();
    device_mac = WiFi.softAPmacAddress();
    mqttClient.setBufferSize(4096);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setKeepAlive(10);

    if (preferences.getString("WIFI_SSID").length() > 1) {
        Serial.println(wifi_ssid);
        wifi_ssid     = preferences.getString("WIFI_SSID");
        wifi_password = preferences.getString("WIFI_PWD");
        Serial.println(wifi_ssid);
        Serial.println(wifi_password);
        Serial.println("Get WIFI INFO From Preference");
    }
    Serial.println(mqtt_broker);
}

void loop()
{
    webServer.handleClient();
    dnsServer.processNextRequest();
    if (WiFi.status() == WL_CONNECTED) {
        if (!mqttClient.connected()) {
            Serial.println("reconnect mqtt...");
            // xSemaphoreTake(xMQTTMutex, portMAX_DELAY);
            mqttConnect(mqtt_broker, mqtt_port, mqtt_id, mqtt_user, mqtt_password, 2000);
            // mqtt_connect_change_event = false;
            // xSemaphoreGive(xMQTTMutex);
        } else {
            mqttClient.loop();
        }
    } else {
        if (wifi_ssid != "") {
            wifiConnect(wifi_ssid, wifi_password, 5000);
        }
    }
    if (M5.Btn.pressedFor(5000)) {
        preferences.clear();
        Serial.println("reset device...");
        esp_restart();
    }
    M5.update();
}

// print bmp

// printer.init();x
// printer.printASCII("M5STACK");
// delay(2000);
// printer.init();
// printer.setBarCodeHRI(ABOVE);
// printer.printBarCode(CODE128, "M5STACK");
// delay(2000);
// printer.init();
// printer.printQRCode("M5STACK");
// delay(2000);
// printer.init();
// printer.printBMP(0, 184, 180, bitbuffer2);