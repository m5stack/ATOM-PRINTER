#ifndef _ATOM_PRINTER_WEB_H
#define _ATOM_PRINTER_WEB_H

#include <WebServer.h>
#include <DNSServer.h>
#include "ATOM_PRINTER_CONFIG.h"
#include <ESPmDNS.h>

extern const IPAddress apIP;

void handleRoot();
void handlerWiFi();
void handleWiFiConfig();
void handleMQTTConfig();
void handlePrint();
void webServerInit();

#endif