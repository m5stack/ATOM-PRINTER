#include "ATOM_PRINTER_WIFI.h"

String wifiScan()
{
    String str;
    WiFi.disconnect();
    delay(100);  // 短暂延迟确保断开连接

    // 执行 WiFi 扫描
    int n = WiFi.scanNetworks();

    // 使用动态数组存储已看到的 SSID
    String seenSSIDs[n];
    int uniqueCount = 0;

    for (int i = 0; i < n; ++i) {
        String currentSSID = WiFi.SSID(i);
        bool isDuplicate   = false;

        // 检查是否已见过此 SSID
        for (int j = 0; j < uniqueCount; ++j) {
            if (currentSSID == seenSSIDs[j]) {
                isDuplicate = true;
                break;
            }
        }

        // 如果是新 SSID，则添加到结果中
        if (!isDuplicate && currentSSID.length() > 0) {
            seenSSIDs[uniqueCount] = currentSSID;
            uniqueCount++;

            str += "<option value=\"";
            str += currentSSID;
            str += "\">";
            str += currentSSID;
            str += "</option>";
        }
    }

    // 如果没有找到网络，添加默认选项
    if (uniqueCount == 0) {
        str += "<option value=\"\">No networks found</option>";
    }

    return str;
}

bool wifiConnect(String _wifi_ssid, String _wifi_password, unsigned long timeout)
{
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
            return is_conneced;
        } else {
            device_state = kInit;
            Serial.print(".");
            device_state = kWiFiDisconnected;
            vTaskDelay(500);
        }
    }
    WiFi.disconnect();
    return is_conneced;
}

void wifiInit()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    String mac_addr = WiFi.softAPmacAddress();
    String No       = mac_addr.substring(mac_addr.length() - 5, mac_addr.length() - 3) +
                mac_addr.substring(mac_addr.length() - 2, mac_addr.length());
    String ssid            = ("ATOM-PRINTER_" + No);
    const char *softAPName = ssid.c_str();
    WiFi.softAP(softAPName);
}
