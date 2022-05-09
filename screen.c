/**
 * @brief CS466 Lab3.c
 *
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define WIDTH   240 //LCD width
#define HEIGHT  320 //LCD height

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "myAssert.h"
#include "fonts.h"

#include "common.h"

#define CS_PIN   5  /* GPIO5 */
#define CLK_PIN  2  /* GPIO2 */
#define MOSI_PIN 3  /* GPIO3 */
#define MISO_PIN 4  /* GPIO4 */

#define LED_PIN 25 // Pico's built in LED
#define DC_PIN 6 // Extra DC pin for display
#define RESET_PIN 7 // Drive this pin LOW to reset display
#define BACKLIGHT 8 // Backlight pin

void drawFrameBuffer(); // Prototype

// DC and RESET should be driven high

// 153.6KB frame buffer
uint16_t *FRAMEBUFFER; // Allocated on the heap

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

// Get a font bitmap for a specific character
int getFont(char c) {
	return c - 32;
}

void drawString(char *phrase, uint16_t posX, uint16_t posY, uint8_t scale) {
    // Composite a string onto the frame buffer in a non-destructive manner
    // Assume the 8 by 8 pixel format
    int char_index = 0;
    if (phrase == NULL) return;
    char current = phrase[char_index++];
    uint8_t mask;
    uint8_t phase = 1;
    while (current != 0) {
        // Draw the current char to frame buffer
		// Loop through all 8 lines for each character
        for (int i = 0; i < 8; i++) {
			// Get the bitmask for current row of pixels
			mask = font[getFont(current) * 8 + i];
			// Loop through each column bit for mask
			for (int j = 0; j < 8; j++) {
				for (int k = 0; k < scale; k++) {
					for (int l = 0; l < scale; l++) {
						// Perform wrap-arounds if out of bounds
						int x = ((WIDTH - ((scale * (posX + j + (phase * 8))) + k)) % WIDTH) + (HEIGHT - WIDTH);
						int y = ((scale * (posY + i)) + l) % HEIGHT;
						int index = y + (x * WIDTH);
						FRAMEBUFFER[index] = (mask & 1<<(7-j)) ? 0xF00F : 0x0;
					}
				}

			}
        }
        phase++;
		current = phrase[char_index++];
	}
	// perform matrix multiplication for scale on matrix section

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

void drawScreen(void *notUsed) {
    /*
	// Need to normalize RGB values
    uint8_t red, blue, green;
    uint8_t redCap = 31;
    uint8_t blueCap = 31;
    uint8_t greenCap = 63;
    red = redCap;
    blue = blueCap;
    green = greenCap;
    //uint16_t pixel = (blue | green<<5 | red<<11);
	*/
	drawString("MPH", 6, 3, 6);
	drawString("69", 10, 15, 6);
	drawString("Battery: 71%", 0, 112, 2);
    while (1) {
		printf("Drawing frame buffer\n");
        drawFrameBuffer();
        vTaskDelay(100);
    }
}

// Reset LCD
static void LCD_2IN_Reset(void)
{
    sleep_ms(100);
    gpio_put(RESET_PIN, LOW);
    sleep_ms(100);
    gpio_put(RESET_PIN, HIGH);
    sleep_ms(100);
}

// Wrapper for writing command
void LCD_2IN_Write_Command(uint8_t address) {
    gpio_put(DC_PIN, LOW);
    uint8_t bytes[1] = {address};
        spi_write_blocking(
            spi0,
            bytes,
            1
        );
}

// Wrapper for writing data
void LCD_2IN_WriteData_Byte(uint8_t byte) {
    gpio_put(DC_PIN, HIGH);
    uint8_t bytes[1] = {byte};
        spi_write_blocking(
            spi0,
            bytes,
            1
        );
}

// Who the fuck knows what this does
void LCD_2IN_Init(void)
{
    LCD_2IN_Reset();

    LCD_2IN_Write_Command(0x36);
    LCD_2IN_WriteData_Byte(0x00);

    LCD_2IN_Write_Command(0x3A);
    LCD_2IN_WriteData_Byte(0x05);

    LCD_2IN_Write_Command(0x21);

    LCD_2IN_Write_Command(0x2A);
    LCD_2IN_WriteData_Byte(0x00);
    LCD_2IN_WriteData_Byte(0x00);
    LCD_2IN_WriteData_Byte(0x01);
    LCD_2IN_WriteData_Byte(0x3F);

    LCD_2IN_Write_Command(0x2B);
    LCD_2IN_WriteData_Byte(0x00);
    LCD_2IN_WriteData_Byte(0x00);
    LCD_2IN_WriteData_Byte(0x00);
    LCD_2IN_WriteData_Byte(0xEF);

    LCD_2IN_Write_Command(0xB2);
    LCD_2IN_WriteData_Byte(0x0C);
    LCD_2IN_WriteData_Byte(0x0C);
    LCD_2IN_WriteData_Byte(0x00);
    LCD_2IN_WriteData_Byte(0x33);
    LCD_2IN_WriteData_Byte(0x33);

    LCD_2IN_Write_Command(0xB7);
    LCD_2IN_WriteData_Byte(0x35);

    LCD_2IN_Write_Command(0xBB);
    LCD_2IN_WriteData_Byte(0x1F);

    LCD_2IN_Write_Command(0xC0);
    LCD_2IN_WriteData_Byte(0x2C);

    LCD_2IN_Write_Command(0xC2);
    LCD_2IN_WriteData_Byte(0x01);

    LCD_2IN_Write_Command(0xC3);
    LCD_2IN_WriteData_Byte(0x12);

    LCD_2IN_Write_Command(0xC4);
    LCD_2IN_WriteData_Byte(0x20);

    LCD_2IN_Write_Command(0xC6);
    LCD_2IN_WriteData_Byte(0x0F);

    LCD_2IN_Write_Command(0xD0);
    LCD_2IN_WriteData_Byte(0xA4);
    LCD_2IN_WriteData_Byte(0xA1);

    LCD_2IN_Write_Command(0xE0);
    LCD_2IN_WriteData_Byte(0xD0);
    LCD_2IN_WriteData_Byte(0x08);
    LCD_2IN_WriteData_Byte(0x11);
    LCD_2IN_WriteData_Byte(0x08);
    LCD_2IN_WriteData_Byte(0x0C);
    LCD_2IN_WriteData_Byte(0x15);
    LCD_2IN_WriteData_Byte(0x39);
    LCD_2IN_WriteData_Byte(0x33);
    LCD_2IN_WriteData_Byte(0x50);
    LCD_2IN_WriteData_Byte(0x36);
    LCD_2IN_WriteData_Byte(0x13);
    LCD_2IN_WriteData_Byte(0x14);
    LCD_2IN_WriteData_Byte(0x29);
    LCD_2IN_WriteData_Byte(0x2D);

    LCD_2IN_Write_Command(0xE1);
    LCD_2IN_WriteData_Byte(0xD0);
    LCD_2IN_WriteData_Byte(0x08);
    LCD_2IN_WriteData_Byte(0x10);
    LCD_2IN_WriteData_Byte(0x08);
    LCD_2IN_WriteData_Byte(0x06);
    LCD_2IN_WriteData_Byte(0x06);
    LCD_2IN_WriteData_Byte(0x39);
    LCD_2IN_WriteData_Byte(0x44);
    LCD_2IN_WriteData_Byte(0x51);
    LCD_2IN_WriteData_Byte(0x0B);
    LCD_2IN_WriteData_Byte(0x16);
    LCD_2IN_WriteData_Byte(0x14);
    LCD_2IN_WriteData_Byte(0x2F);
    LCD_2IN_WriteData_Byte(0x31);
    LCD_2IN_Write_Command(0x21);

    LCD_2IN_Write_Command(0x11);

    LCD_2IN_Write_Command(0x29);
}

void LCD_2IN_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t  Yend)
{
    LCD_2IN_Write_Command(0x2a);
    LCD_2IN_WriteData_Byte(Xstart >>8);
    LCD_2IN_WriteData_Byte(Xstart & 0xff);
    LCD_2IN_WriteData_Byte((Xend - 1) >> 8);
    LCD_2IN_WriteData_Byte((Xend - 1) & 0xff);

    LCD_2IN_Write_Command(0x2b);
    LCD_2IN_WriteData_Byte(Ystart >>8);
    LCD_2IN_WriteData_Byte(Ystart & 0xff);
    LCD_2IN_WriteData_Byte((Yend - 1) >> 8);
    LCD_2IN_WriteData_Byte((Yend - 1) & 0xff);

    LCD_2IN_Write_Command(0x2C);
}

void DEV_SPI_Write_nByte(uint8_t *data, uint32_t length) {
    gpio_put(DC_PIN, HIGH);
        spi_write_blocking(
            spi0,
            data,
            length
        );
}

void LCD_2IN_Clear(uint16_t Color)
{
    uint16_t i;
    uint16_t image[WIDTH];
    for(i=0;i<WIDTH;i++){
        image[i] = Color>>8 | (Color&0xff)<<8;
    }
    uint8_t *p = (uint8_t *)(image);
    LCD_2IN_SetWindow(0, 0, WIDTH, HEIGHT);
    gpio_put(DC_PIN, 1);
	for (int i = 0; i < WIDTH; )
    for(i = 0; i < HEIGHT; i++){
        DEV_SPI_Write_nByte(p,WIDTH*2);
    }
}

// Draws the frame buffer to the display
void drawFrameBuffer()
{
    LCD_2IN_SetWindow(0, 0, WIDTH, HEIGHT);
    for(int i = 0; i < HEIGHT; i++){
        DEV_SPI_Write_nByte((uint8_t *)FRAMEBUFFER, WIDTH * HEIGHT * 2);
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
    spi_init(spi0, 145000000); // SPI0 at 1kHz
    spi_set_format(
        spi0,
        8, // 8 bits per transfer
        0x00000080,
        0x00000040,
        SPI_MSB_FIRST
    );

    LCD_2IN_Init();
    FRAMEBUFFER = malloc(sizeof(uint16_t) * WIDTH * HEIGHT); // PLEASE WORK
	memset(FRAMEBUFFER, 0, sizeof(uint16_t) * WIDTH * HEIGHT);
    myAssert(FRAMEBUFFER != NULL);
    LCD_2IN_Clear(0xFFFF);
    // Create idle task for heartbeat
    xTaskCreate(heartbeat, "heartbeat", 256, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(drawScreen, "draw", 600, NULL, 1, NULL);
    //xTaskCreate(gpioVerifyReadWrite, "verify", 256, NULL, tskIDLE_PRIORITY, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){};
}
