#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "base/base.h"
#include "homie/homie.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#define CONTROLLER_STATE_FILTER "+/5/+/$state"
#define CONTROLLER_DEVICE_MAX 64
#define CONTROLLER_LINE_MAX 256
#define CONTROLLER_VALUE_MAX 64
#define CONTROLLER_HTTP_CONNECTION_MAX 8

typedef struct ControllerValue ControllerValue;
struct ControllerValue 
{
    u8 bytes[CONTROLLER_VALUE_MAX];
    u64 length;
    b32 is_known;
};

typedef struct ControllerDevice ControllerDevice;
struct ControllerDevice
{
    HomieId domain;
    HomieId id;
    HomieState state;
    b32 is_described;
    HomieDescription description;
    ControllerValue values[HOMIE_PROPERTY_MAX];
};

typedef struct ControllerHttpConnection ControllerHttpConnection;
struct ControllerHttpConnection 
{
    b32 is_open;
    int sock;
    u64 opened_ms;
};


typedef struct Controller Controller;
struct Controller
{
    MqttClient client;
    Loop loop;
    ControllerDevice devices[CONTROLLER_DEVICE_MAX];
    u8 line[CONTROLLER_LINE_MAX];
    u64 line_length;
    int http_sock;
    ControllerHttpConnection http[CONTROLLER_HTTP_CONNECTION_MAX];
};

ControllerDevice *controller_find(Controller *c, const HomieTopic *t);
ControllerDevice *controller_find_id(Controller *c, str id);
ControllerDevice *controller_add(Controller *c, const HomieTopic *t);
i32 controller_property_find(const ControllerDevice *d, str node, str id);
void controller_list(const Controller *c);
b32 controller_set(Controller *c, str device, str property, str value);

b32 controller_on_publish(Controller *c, MqttPublish m);
void controller_on_value(Controller *c, const HomieTopic *t, str payload);
b32 controller_read_input(Controller *c, b32 *is_closed);

b32 controller_http_listen(Controller *c, const char *ip, u16 port);
void controller_http_handle(Controller *c);

#endif
