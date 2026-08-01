#include "base/base_types.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEVICE_SIM_BROKER_IP "127.0.0.1"
#define DEVICE_SIM_BROKER_PORT 1883
#define DEVICE_SIM_ID "tnn-sim-1"
#define DEVICE_SIM_TOPIC "homie/5/" DEVICE_SIM_ID "/"

int main(void)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    int one = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct sockaddr_in broker = {0};
    broker.sin_family = AF_INET;
    broker.sin_port = htons(DEVICE_SIM_BROKER_PORT);
    inet_pton(AF_INET, DEVICE_SIM_BROKER_IP, &broker.sin_addr);

    if (connect(sock, (struct sockaddr*) &broker, sizeof(broker)) < 0)
    {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("connected to %s:%d\n", DEVICE_SIM_BROKER_IP, DEVICE_SIM_BROKER_PORT);

    u8 connect_packet [] = {
        0x10, 0x15,
        0x00, 0x04, 'M', 'Q', 'T', 'T',
        0x04,
        0x02,
        0x00, 0x00,
        0x00, 0x09, 't', 'n', 'n', '-', 's', 'i', 'm', '-', '1'
    };
    send(sock, connect_packet, sizeof(connect_packet), 0);

    u8 connection_ack[4];
    ssize_t received = recv(sock, connection_ack, sizeof(connection_ack), 0);
    if (received != 4 || connection_ack[0] != 0x20)
    {
        fprintf(stderr, "expected a 4-byte CONNACK, got %zd bytes\n", received);
        close(sock);
        return 1;
    }
    if (connection_ack[3] != 0)
    {
        fprintf(stderr, "CONNACK: refused (return code %d)\n", connection_ack[3]);
        close(sock);
        return 1;
    }
    printf("CONNACK: accepted (return code 0)\n");

    close(sock);
    return 0;
}