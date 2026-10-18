#ifndef MQTT_ENCODE_H
#define MQTT_ENCODE_H

#include "base/base.h"

void mqtt_write_str(Writer *w, str value);
void mqtt_write_varint(Writer *w, u32 value);

void mqtt_write_connect(Writer *w, str client_id, u16 keep_alive_seconds, str will_topic, str will_payload);
void mqtt_write_publish(Writer *w, str topic, str payload, b32 retain);
void mqtt_write_subscribe(Writer *w, u16 packet_id, str topic_filter);
void mqtt_write_unsubscribe(Writer *w, u16 packet_id, str topic_filter);
void mqtt_write_pingreq(Writer *w);
void mqtt_write_disconnect(Writer *w);

#endif