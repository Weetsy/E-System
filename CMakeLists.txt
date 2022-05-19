
cmake_minimum_required(VERSION 3.13)

include(~/src/pico/pico-sdk/external/pico_sdk_import.cmake)
#include(external/pico_sdk_import.cmake)

set(PICO_SDK_FREERTOS_SOURCE ~/src/FreeRTOSv202112.00/FreeRTOS/Source)

project(screen C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.3.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.3.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )


add_library(freertos
        ${PICO_SDK_FREERTOS_SOURCE}/event_groups.c
        ${PICO_SDK_FREERTOS_SOURCE}/list.c
        ${PICO_SDK_FREERTOS_SOURCE}/queue.c
        ${PICO_SDK_FREERTOS_SOURCE}/stream_buffer.c
        ${PICO_SDK_FREERTOS_SOURCE}/tasks.c
        ${PICO_SDK_FREERTOS_SOURCE}/timers.c
        ${PICO_SDK_FREERTOS_SOURCE}/portable/MemMang/heap_3.c
        ${PICO_SDK_FREERTOS_SOURCE}/portable/GCC/ARM_CM0/port.c
)

target_include_directories(freertos PUBLIC
        .
        ${PICO_SDK_FREERTOS_SOURCE}/include
        ${PICO_SDK_FREERTOS_SOURCE}/portable/GCC/ARM_CM0
)


add_executable(screen
        screen.c
        myAssert.c
        fonts.c
        LCD_2IN.c
        Pico_UPS.c
        )

# pull in common dependencies
target_link_libraries(screen pico_stdlib freertos hardware_spi hardware_i2c)

# tell the pico library that you will be using usb serial and not an actual uart on the
# processor
pico_enable_stdio_usb(screen 1)
pico_enable_stdio_uart(screen 0)


# create map/bin/hex file etc.
pico_add_extra_outputs(screen)

add_custom_command(TARGET screen POST_BUILD COMMAND arm-none-eabi-size screen.elf)

# add url via pico_set_program_url
# example_auto_set_url(blink)
