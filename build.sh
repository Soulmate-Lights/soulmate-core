#!/bin/bash

N_CORES=$(sysctl -n hw.ncpu) # macOS
# TODO: get cores linux

# ESP-IDF v3.x
esp_idf_v3_build_native() {
    cp arduinojason.mk components/ArduinoJson/component.mk
    make all -j$N_CORES;
    rm components/ArduinoJson/component.mk
}

# build docker v3.x
esp_idf_v3_build_docker() {
    cp arduinojason.mk components/ArduinoJson/component.mk
    docker run --rm -v $PWD:/project -w /project espressif/idf:v3.3.2 make all -j$N_CORES;
    rm components/ArduinoJson/component.mk
}

# flash v3.x
esp_idf_v3_flash() {
    python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 $@ --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0xe000 build/ota_data_initial.bin 0x1000 build/bootloader/bootloader.bin 0x10000 build/soulmate.bin 0x8000 build/partitions.bin
}

# ESP-IDF v4.x
# build docker v4.x
esp_idf_v4_build_docker() {
    docker run --rm -v $PWD:/project -w /project espressif/idf:release-v4.3 idf.py build;
}

if [[ "$1" == "docker" ]]; then
    esp_idf_v3_build_docker
elif [[ "$1" == "flash" ]]; then
    esp_idf_v3_flash ${@:2}
else
    esp_idf_v3_build_native
fi
