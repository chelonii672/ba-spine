#!/bin/sh

mkdir -p out/
g++ src/main.cpp -o out/exporter -O2 -std=c++2a -I lib/spine/include lib/spine/src/spine/*.cpp -lsfml-system -lsfml-graphics