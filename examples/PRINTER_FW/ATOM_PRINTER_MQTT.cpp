#include "ATOM_PRINTER_MQTT.h"


bool mqttConnect(String _mqtt_broker, int _mqtt_port, String _mqtt_id,
                 String _mqtt_user, String _mqtt_password,
                 unsigned long _timeout) {
    unsigned long start = millis();
    bool is_conneced    = false;

    mqttClient.disconnect();
    mqttClient.setServer(_mqtt_broker.c_str(), _mqtt_port);
    Serial.print(F("Connecting"));
    String mqttid;

    if (_mqtt_broker.indexOf("m5stack") != -1) {
        mqttid = ("MQTTID_" + String(random(65536)));
    } else {
        mqttid = _mqtt_id;
    }

    if (mqtt_topic == ""){
        mqtt_topic = mac_addr;
    }

    while (millis() - start < _timeout) {
        if(mqttClient.connect(mqttid.c_str(), _mqtt_user.c_str(), _mqtt_password.c_str())) {
            Serial.println("MQTT Connected!");
            is_conneced = true;
            Serial.println(F(" success"));
            printer.printASCII(F("successfully connect to UIFLOW"));
            printer.newLine(3);
            printer.printASCII("subscribe: " + mqtt_topic);
            Serial.println("subscribe: " + mqtt_topic);
            mqttClient.subscribe(mqtt_topic.c_str());
            break;
        }else{
            Serial.print(".");
            delay(500);
        }
    }

    if(is_conneced) {
        preferences.putString("MQTT_BROKER", _mqtt_broker);
        preferences.putInt("MQTT_PORT", _mqtt_port);
        preferences.putString("MQTT_ID", _mqtt_id);
        preferences.putString("MQTT_USER", _mqtt_user);
        preferences.putString("MQTT_PASSPWD", _mqtt_password);
        preferences.putString("MQTT_TOPIC", mqtt_topic);
        mqtt_broker = _mqtt_broker;
        mqtt_port = _mqtt_port;
        mqtt_id = _mqtt_id;
        mqtt_user = _mqtt_user;
        mqtt_password = _mqtt_password;
        device_state = kMQTTConnected;
    } else {
        device_state = kMQTTDisconnected;
    }

    return is_conneced;
}
