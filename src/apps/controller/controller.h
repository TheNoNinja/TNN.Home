#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "base/base.h"
#include "homie/homie.h"
#include "loop/loop.h"
#include "mqtt/mqtt.h"
#include "http/http.h"

#define CONTROLLER_DEVICE_MAX 64
#define CONTROLLER_COMMAND_MAX 256
#define CONTROLLER_HTTP_CONNECTION_MAX 8
#define CONTROLLER_HTTP_REQUEST_MAX KB(8)
#define CONTROLLER_HTTP_PAGE_MAX KB(1024)

typedef struct ControllerDevice ControllerDevice;
struct ControllerDevice
{
    HomieId domain;
    HomieId id;
    HomieState state;
    b32 is_described;
    HomieDescription description;
    HomieValue values[HOMIE_PROPERTY_MAX];
};

typedef struct ControllerHttpConnection ControllerHttpConnection;
struct ControllerHttpConnection 
{
    b32 is_open;
    b32 is_stream;
    int sock;
    u64 opened_ms;
    u64 length;
    u8 request[CONTROLLER_HTTP_REQUEST_MAX];
};

enum ControllerSetResult
{
    CONTROLLER_SET_SENT,
    CONTROLLER_SET_REFUSED,
    CONTROLLER_SET_MALFORMED,
    CONTROLLER_SET_LOST,
};
typedef enum ControllerSetResult ControllerSetResult;

typedef struct Controller Controller;
struct Controller
{
    MqttClient client;
    ControllerDevice devices[CONTROLLER_DEVICE_MAX];
    u8 line[CONTROLLER_COMMAND_MAX];
    u64 line_length;
    int http_sock;
    str http_ip;
    u16 http_port;
    ControllerHttpConnection http[CONTROLLER_HTTP_CONNECTION_MAX];
};

ControllerDevice *controller_device_find(Controller *c, const HomieTopic *t);
ControllerDevice *controller_device_find_id(Controller *c, str id);
ControllerDevice *controller_device_add(Controller *c, const HomieTopic *t);
void controller_devices_list(const Controller *c);
ControllerSetResult controller_set(Controller *c, str words);

b32 controller_on_publish(Controller *c, MqttPublish m);
void controller_on_value(Controller *c, const HomieTopic *t, str payload);
b32 controller_commands_read(Controller *c, b32 *is_closed);

b32 controller_http_listen(Controller *c, const char *ip, u16 port);
b32 controller_http_handle(Controller *c);
void controller_http_close(Controller *c, ControllerHttpConnection *h);

void controller_events_open(Controller *c, ControllerHttpConnection *h);
void controller_events_device_changed(Controller *c, const ControllerDevice *d);
void controller_events_device_removed(Controller *c, const ControllerDevice *d);
void controller_events_heartbeat(Controller *c);

void controller_page_write(const Controller *c, Writer *w);
void controller_page_device(Writer *w, const ControllerDevice *d);
void controller_page_removed(Writer *w, const ControllerDevice *d);

#endif
