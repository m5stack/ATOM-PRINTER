#ifndef _ATOM_PRINTER_WIFI_H
#define _ATOM_PRINTER_WIFI_H

#include <WiFi.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include "ATOM_PRINTER_CONFIG.h"

extern String ssid_html;
extern String mac_addr;
extern const IPAddress apIP;
extern Preferences preferences;
extern String wifi_ssid;
extern String wifi_password;
extern Atom_Printer_State_t device_state;

String wifiScan();
bool wifiConnect(String ssid, String password, unsigned long timeout);
void setWifiMode();

#endif