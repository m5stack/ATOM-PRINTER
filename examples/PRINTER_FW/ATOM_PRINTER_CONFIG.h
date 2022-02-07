#ifndef _ATOM_PRINTER_CONFIG_H
#define _ATOM_PRINTER_CONFIG_H


#define DNS_PORT 53
#define BMP_BUFFER_LIMIT 1024 * 50
#define MQTT_BROKER "mqtt.m5stack.com"
#define MQTT_PORT 1883
#define MQTT_ID "printer"

typedef enum {
    kInit = 0,
    kWiFiConnected,
    kMQTTConnected,
} Atom_Printer_State_t;

#endif