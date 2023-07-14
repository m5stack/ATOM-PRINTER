#ifndef _ATOM_PRINTER_CONFIG_H
#define _ATOM_PRINTER_CONFIG_H

#define DNS_PORT         53
#define BMP_BUFFER_LIMIT 1024 * 50
#define MQTT_BROKER      "mqtt.m5stack.com"
#define MQTT_PORT        1883
#define MQTT_ID          "printer"
#define MQTT_USER        "USER"
#define MQTT_PASSWORD    "PASSWORD"
#define MQTT_TOPIC       ""  // if "" default use the mac address for subscribe

typedef enum {
    kInit = 0,
    kWiFiConnected,
    kWiFiDisconnected,
    kMQTTConnected,
    kMQTTDisconnected,
} Atom_Printer_State_t;

#endif