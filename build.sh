#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
    N_CORES=$(sysctl -n hw.ncpu)
elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
    N_CORES=$(nproc)
fi

copy_overrides_files() {
    cmp --silent overrides/arduinoJson.mk components/ArduinoJson/component.mk || cp overrides/arduinoJson.mk components/ArduinoJson/component.mk
    cmp --silent overrides/arduino.mk components/arduino/component.mk || cp overrides/arduino.mk components/arduino/component.mk
}

restore_ovrerides_files() {
    (cd components/ArduinoJson && rm component.mk)
    (cd components/arduino && git checkout component.mk)
}

# The v3.3.4 image ships ccache 3.4.1, but IDF's make build system never calls
# it. These masquerade symlinks sit ahead of the toolchain in PATH so every
# compile routes through ccache. Done this way rather than via CC because
# project.mk runs `which $(CC)`, which a "ccache xtensa-esp32-elf-gcc" value
# would break. Single-quoted: $t and $PATH must expand inside the container.
# The cache lives in a named volume so it survives --rm.
CCACHE_SHIM='mkdir -p /tmp/ccache-shim && for t in gcc g++ c++; do ln -sf /usr/bin/ccache /tmp/ccache-shim/xtensa-esp32-elf-$t; done && export PATH=/tmp/ccache-shim:$PATH'

# ESP-IDF v3.x
esp_idf_v3_build_native() {
    copy_overrides_files

    # Run build. `make defconfig` first: it generates each component's
    # component_project_vars.mk, which exports that component's include dirs to
    # its dependents. Without it, a parallel build on a cold tree races those
    # files and IDF's own sources fail to find IDF's own headers.
    make defconfig && make all -j$N_CORES;
    STATUS=$?

    restore_ovrerides_files

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

    copy_overrides_files

    # Run build
    docker run --rm -v kconfig:/opt/esp/idf/tools/kconfig \
        -v soulmate-ccache:/ccache -e CCACHE_DIR=/ccache \
        -v $PWD:/project -w /project espressif/idf:v3.3.4 \
        bash -c "$CCACHE_SHIM && make defconfig && make all -j$N_CORES_DOCKER && ccache -s";
    STATUS=$?

    restore_ovrerides_files

    # Verify build success
    if [ $STATUS -ne 0 ]; then
        echo "[ERROR] Docker Build Failed"
        exit $STATUS
    fi
}

# flash v3.x
esp_idf_v3_flash() {
    python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 $@ --baud 1500000 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0xe000 build/ota_data_initial.bin 0x1000 build/bootloader/bootloader.bin 0x10000 build/soulmate.bin 0x8000 build/partitions.bin
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
