# TNN.Home

## Prerequisites

### Development 
- Docker

### Release
TBA

## Development

### MQTT Brokers
Setup broker containers once
```bash
docker run -d -t --name tnn-nanomq -p 1883:1883 docker.io/emqx/nanomq:0.25.6-slim
docker run -d -t --name tnn-mosquitto -p 1884:1883 docker.io/library/eclipse-mosquitto:2.1.2-alpine
```
It it exists you can start it again with
```bash
docker start tnn-nanomq
docker start tnn-mosquitto
```

### Building
GCC
```bash
cmake --preset debug && cmake --build --preset debug
```

Clang
```bash
cmake --preset debug-clang && cmake --build --preset debug-clang
```

Release unity
```bash
cmake --preset release-unity && cmake --build --preset release-unity
```
