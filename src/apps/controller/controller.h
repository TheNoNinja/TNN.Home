#ifndef CONTROLLER_CONTROLLER_H
#define CONTROLLER_CONTROLLER_H

#include "base/base.h"
#include "homie/homie.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"

#define CONTROLLER_STATE_FILTER "+/5/+/$state"
#define CONTROLLER_DEVICE_MAX 64
#define CONTROLLER_LINE_MAX 128

typedef struct ControllerDevice ControllerDevice;
struct ControllerDevice {
    HomieId domain;
    HomieId id;
    HomieState state;
    u64 description_length;
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
void controller_list(const Controller *c);

b32 controller_on_publish(Controller *c, MqttPublish m);
b32 controller_read_input(Controller *c);

#endif
