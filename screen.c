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
#include "hardware/sync.h" // Include sync for spinlocks
#include "pico/stdlib.h"
#include "myAssert.h"

#include "common.h"

#define LED_PIN 25 // Pico's built in LED

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

    // Set up GPIO pins as output from pico
    gpio_set_dir(LED_PIN, GPIO_OUT);

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
        printf("hb-tick: %d\n", 500); // 1Hz blinking
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

    // Create idle task for heartbeat
    xTaskCreate(heartbeat, "heartbeat", 256, NULL, tskIDLE_PRIORITY, NULL);
    //xTaskCreate(gpioVerifyReadWrite, "verify", 256, NULL, tskIDLE_PRIORITY, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){};
}
