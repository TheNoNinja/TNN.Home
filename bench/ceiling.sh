#!/usr/bin/env bash

container="$1"
port="${2:-1883}"

tool=$(command -v podman.exe || command -v docker)

for clients in 1 10; do
    echo "$clients piblisher(s) -> $clients subscriber(s), 16-byte QoS 0 messages, 10s:"
    "$tool" run --rm --network "container:$container" docker.io/emqx/nanomq:0.25.6-full sh -c "
        timeout 12 stdbuf -oL nanomq_cli bench sub -p $port -c $clients -t 'bench/%i' > /tmp/sub.txt &
        sleep 1
        timeout 10 stdbuf -oL nanomq_cli bench pub -p $port -c $clients -I 0 -s 16 -t 'bench/%i' > /tmp/pub.txt
        wait
        grep sent: /tmp/pub.txt | tail -n 1
        grep recv: /tmp/sub.txt"
done