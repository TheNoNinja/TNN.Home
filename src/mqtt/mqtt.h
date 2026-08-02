#ifndef MQTT_H
#define MQTT_H

#include "mqtt/mqtt_reader.h"
#include "mqtt/mqtt_writer.h"

#define MQTT_CONNECT     1
#define MQTT_CONNECT_ACK 2
#define MQTT_PUBLISH     3
#define MQTT_PINGREQ     12
#define MQTT_PINGRESP    13
#define MQTT_DISCONNECT  14

#define MQTT_VARINT_MAX  268435455

#define MQTT_CONNECT_FLAG_CLEAN_SESSION 0x02
#define MQTT_CONNECT_FLAG_WILL 0x04
#define MQTT_CONNECT_FLAG_WILL_RETAIN 0x20

#define MQTT_PROTOCOL_3_1_1 4

#endif