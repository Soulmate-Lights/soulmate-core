#!/bin/bash

N_CORES=$(sysctl -n hw.ncpu) # macOS
# TODO: get cores linux

# ESP-IDF v3.x
esp_idf_v3_build_native() {
    cp arduinojason.mk components/ArduinoJson/component.mk
    make defconfig all -j$N_CORES;
    rm components/ArduinoJson/component.mk
}

# v3.x build docker
esp_idf_v3_build_docker() {
    cp arduinojason.mk components/ArduinoJson/component.mk
    docker run --rm -v $PWD:/project -w /project espressif/idf:v3.3.2 make defconfig all -j$N_CORES;
    rm components/ArduinoJson/component.mk
}

# ESP-IDF v4.x build docker
esp_idf_v4_build_docker() {
    docker run --rm -v $PWD:/project -w /project espressif/idf:release-v4.3 idf.py build;
}

if [[ "$1" == "docker" ]]; then
    esp_idf_v3_build_docker
else
    esp_idf_v3_build_native
fi
