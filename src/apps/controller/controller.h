#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "base/base.h"
#include "homie/homie.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#define CONTROLLER_STATE_FILTER "+/5/+/$state"
#define CONTROLLER_DEVICE_MAX 64
#define CONTROLLER_LINE_MAX 128
#define CONTROLLER_VALUE_MAX 64

typedef struct ControllerValue ControllerValue;
struct ControllerValue 
{
    u8 bytes[CONTROLLER_VALUE_MAX];
    u64 length;
    b32 known;
};

typedef struct ControllerDevice ControllerDevice;
struct ControllerDevice
{
    HomieId domain;
    HomieId id;
    HomieState state;
    b32 is_described;;
    HomieDescription description;
    ControllerValue values[HOMIE_PROPERTY_MAX];
};

typedef struct Controller Controller;
struct Controller
{
    MqttClient client;
    Loop loop;
    ControllerDevice devices[CONTROLLER_DEVICE_MAX];
    u8 line[CONTROLLER_LINE_MAX];
    u64 line_length;
};

ControllerDevice *controller_find(Controller *c, const HomieTopic *t);
ControllerDevice *controller_add(Controller *c, const HomieTopic *t);
i32 controller_property_find(const ControllerDevice *d, str node, str id);
void controller_list(const Controller *c);

b32 controller_on_publish(Controller *c, MqttPublish m);
b32 controller_read_input(Controller *c);

#endif
