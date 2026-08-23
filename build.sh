#!/usr/bin/env bash

cmake --preset debug && cmake --preset debug-clang && cmake --preset release-unity && cmake --preset fuzz && cmake --build --preset debug && cmake --build --preset debug-clang && cmake --build --preset release-unity && cmake --build --preset fuzz