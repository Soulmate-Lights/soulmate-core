#!/bin/bash

N_CORES=$(sysctl -n hw.ncpu) # macOS
# TODO: get cores linux

# ESP-IDF v3.x
esp_idf_v3_build_native() {
    # [ArduinoJson] No makefile workaround begin
    echo "COMPONENT_ADD_INCLUDEDIRS := src" > components/ArduinoJson/component.mk

    # Run build
    make all -j$N_CORES;
    STATUS=$?

    # [ArduinoJson] No makefile workaround end
    rm components/ArduinoJson/component.mk

    # Verify build success
    if [ $STATUS -ne 0 ]; then
        echo "[ERROR] Build Failed"
        exit $STATUS
    fi
}

# build docker v3.x
esp_idf_v3_build_docker() {
    # get number of docker machine cores
    N_CORES_DOCKER=$(docker run --rm espressif/idf:v3.3.4 /bin/bash -c "grep processor /proc/cpuinfo | wc -l; exit" | tail -1)

    # [ArduinoJson] No makefile workaround begin
    echo "COMPONENT_ADD_INCLUDEDIRS := src" > components/ArduinoJson/component.mk

    # Run build
    docker run --rm -v kconfig:/opt/esp/idf/tools/kconfig -v $PWD:/project -w /project espressif/idf:v3.3.4 make all -j$N_CORES_DOCKER;
    STATUS=$?

    # [ArduinoJson] No makefile workaround end
    rm components/ArduinoJson/component.mk

    # Verify build success
    if [ $STATUS -ne 0 ]; then
        echo "[ERROR] Docker Build Failed"
        exit $STATUS
    fi
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
