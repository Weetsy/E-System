#!/bin/bash

# Compile libraries
#cd misc/yaml-cpp-0.6.3/
#mkdir build
#cd build
#cmake ..
#make
#cd ../../../

# Setup Build Directories
if [ ! -d build ]; then
    mkdir build
fi
cd build
PICO_SDK_PATH="~/src/pico/pico-sdk"
# If -d flag is present, build in debug mode
if [ "$1" = "-d" ] || [ "$2" = "-d" ]; then
    echo "RUNNING UNDER DEBUG MODE"
    ARGS="-DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Debug -DPICO_SDK_PATH=$PICO_SDK_PATH"
else
    echo "RUNNING UNDER RELEASE MODE"
    ARGS="-Wno-dev -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release -DPICO_SDK_PATH=$PICO_SDK_PATH"
fi
cmake ${ARGS} ..

# Build Project
TYPE=`uname`
if [ ${TYPE} = "Darwin" ]; then
    make -j$(sysctl -n hw.physicalcpu)
else
    make -j$(nproc)
fi
if [ $? != 0 ]; then
    echo -e "\033[0;31m --- Build errors detected! ---"
else
    if [ "$1" = "-t" ] || [ "$2" = "-t" ]; then
        #ctest --output-on-failure
        echo "Tests NOT supported yet"
    else
        cp screen.uf2 /media/weetsy/RPI-RP2/
    fi
fi
