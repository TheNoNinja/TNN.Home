# TNN.Home

## Prerequisites

### Development 
- Docker

### Release
TBA

## Development

### MQTT Broker
Setup broker container once
```bash
docker run -d -t --name tnn-nanomq -p 1883:1883 docker.io/emqx/nanomq:0.25.6-slim
```
It it exists you can start it again with
```bash
docker start tnn-nanomq
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
