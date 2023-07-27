#ifndef _ATOM_PRINTER_MQTT_H
#define _ATOM_PRINTER_MQTT_H

#include "ATOM_PRINTER.h"
#include <PubSubClient.h>
#include "ATOM_PRINTER_CONFIG.h"
#include <Preferences.h>

extern ATOM_PRINTER printer;
extern PubSubClient mqttClient;
extern Preferences preferences;
extern String device_mac;

extern String mqtt_broker;
extern int mqtt_port;
extern String mqtt_id;
extern String mqtt_user;
extern String mqtt_password;
extern String mqtt_topic;
extern Atom_Printer_State_t device_state;


bool mqttConnect(String mqtt_broker, int mqtt_port, String mqtt_id,
                 String mqtt_user, String mqtt_password, unsigned long timeout);

#endif
