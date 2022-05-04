/**
 * @brief CS466 Lab3.c
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "myAssert.h"

#include "common.h"

#define CS_PIN   5  /* GPIO5 */
#define CLK_PIN  2  /* GPIO2 */
#define MOSI_PIN 3  /* GPIO3 */
#define MISO_PIN 4  /* GPIO4 */

#define LED_PIN 25 // Pico's built in LED
#define DC_PIN 6 // Extra DC pin for display
#define RESET_PIN 7 // Drive this pin LOW to reset display
#define BACKLIGHT 8 // Backlight pin

// DC and RESET should be driven high

/*
 (void) led_control powers the LED on LED_PIN when (bool) isOn is true, and
 powers it off when false.
*/
void led_control(bool isOn)
{
    isOn ? gpio_put(LED_PIN, HIGH) : gpio_put(LED_PIN, LOW);
}

// Callback function for button presses
void gpio_int_callback(uint gpio, uint32_t events_unused)
{

}

// Initialize GPIO pins on pico board
void hardware_init(void)
{
    // Initialize GPIO pins on pico
    gpio_init(LED_PIN);
    gpio_init(RESET_PIN);
    gpio_init(DC_PIN);

    // Set up GPIO pins as output from pico
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_set_dir(DC_PIN, GPIO_OUT);

    gpio_set_function(CS_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    gpio_put(DC_PIN, HIGH);
    gpio_put(RESET_PIN, HIGH);

}

/*
 (void) heartbeat is an idle task that occassionally polls the switch connected
 to the expander on GPIOB and lights the remote led if the switch is pressed.
 When the switch is not pressed, the LED on the pico blinks at 1hz and the
 remote LED is powered off.
*/
void heartbeat(void *notUsed)
{
    while (true)
    {
        uint16_t shorts[5] = {100, 200, 300, 400, 500};
        int res = spi_write16_blocking(
            spi0,
            shorts,
            5
        );
        printf("hb-tick: %d, wrote %d\n", 500, res); // 1Hz blinking
        // Blink for 1Hz
        led_control(true);
        vTaskDelay(500 / portTICK_PERIOD_MS);
        led_control(false);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

int main()
{
    // Initialize stdio
    stdio_init_all();
    printf("lab2 Hello!\n");

    // Initialize hardware
    hardware_init();

    // Initialize SPI
    spi_init(spi0, 1000); // SPI0 at 1kHz
    spi_set_format(
        spi0,
        16, // 16 bits per transfer
        0x00000080,
        0x00000040,
        SPI_MSB_FIRST
    );

    // Create idle task for heartbeat
    xTaskCreate(heartbeat, "heartbeat", 256, NULL, tskIDLE_PRIORITY, NULL);
    //xTaskCreate(gpioVerifyReadWrite, "verify", 256, NULL, tskIDLE_PRIORITY, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){};
}
