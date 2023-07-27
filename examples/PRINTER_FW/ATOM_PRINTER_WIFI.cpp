#include "ATOM_PRINTER_WIFI.h"

String wifiScan() {
    // 获取当前搜索到的wifi名称列表
    String str;
    WiFi.disconnect();
    int n = WiFi.scanNetworks();
    for (int i = 0; i < n; ++i) {
        str += "<option value=\"";
        str += WiFi.SSID(i);
        str += "\">";
        str += WiFi.SSID(i);
        str += "</option>";
    }
    return str;
}

bool wifiConnect(String _wifi_ssid, String _wifi_password,
                 unsigned long timeout) {
    WiFi.disconnect();
    WiFi.begin(_wifi_ssid.c_str(), _wifi_password.c_str());
    unsigned long start = millis();
    bool is_conneced    = false;
    while (millis() - start < timeout) {
        if ((WiFi.status() == WL_CONNECTED)) {
            Serial.println("WiFi Connected!");
            is_conneced   = true;
            wifi_ssid     = _wifi_ssid;
            wifi_password = _wifi_password;
            preferences.putString("WIFI_SSID", _wifi_ssid);
            preferences.putString("WIFI_PWD", _wifi_password);
            device_state = kWiFiConnected;
            break;
        } else {
            device_state = kInit;
            Serial.print(".");
            device_state = kWiFiDisconnected;
            vTaskDelay(500);
        }
    }
    return is_conneced;
}

void wifiInit() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    String mac_addr  = WiFi.softAPmacAddress();
    String No =
        mac_addr.substring(mac_addr.length() - 5, mac_addr.length() - 3) +
        mac_addr.substring(mac_addr.length() - 2, mac_addr.length());
    String ssid            = ("ATOM-PRINTER_" + No);
    const char *softAPName = ssid.c_str();
    WiFi.softAP(softAPName);
}
