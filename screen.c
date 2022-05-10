// screen.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "myAssert.h"
#include "fonts.h"
#include "LCD_2IN.h"
#include "common.h"

#define SW1 14
#define SW2 15

void drawFrameBuffer(); // Prototype

// DC and RESET should be driven high

// 153.6KB frame buffer
uint16_t FRAMEBUFFER[WIDTH * HEIGHT];
QueueHandle_t queue;
int speed;

/*
 (void) led_control powers the LED on LED_PIN when (bool) isOn is true, and
 powers it off when false.
*/
void led_control(bool isOn)
{
    isOn ? gpio_put(LED_PIN, HIGH) : gpio_put(LED_PIN, LOW);
}

// Initialize GPIO pins on pico board
void hardware_init(void)
{
    // Initialize GPIO pins on pico
    gpio_init(LED_PIN);
    gpio_init(RESET_PIN);
    gpio_init(DC_PIN);
    gpio_init(SW1);
    gpio_init(SW2);

    // Set up GPIO pins as output from pico
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_set_dir(DC_PIN, GPIO_OUT);

    gpio_set_dir(SW1, GPIO_IN);
    gpio_set_dir(SW2, GPIO_IN);

    gpio_pull_up(SW1);
    gpio_pull_up(SW2);

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
	char currentSpeed[10];
    while (1) {
        memset(FRAMEBUFFER, 0, WIDTH * HEIGHT * sizeof(uint16_t));
        sprintf(currentSpeed, "%d", speed);
        drawString("MPH", 6, 3, 6);
    	drawString(currentSpeed, 10, 15, 6);
    	drawString("Battery: 71%", 0, 112, 2);
		printf("Drawing frame buffer\n");
        drawFrameBuffer();
        vTaskDelay(100);
    }
}

// Draws the frame buffer to the display
void drawFrameBuffer()
{
    LCD_2IN_SetWindow(0, 0, WIDTH, HEIGHT);
    DEV_SPI_Write_nByte((uint8_t *)FRAMEBUFFER, WIDTH * HEIGHT * 2);
}

void changeSpeed(void *notUsed) {
    while (1) {
        if (!gpio_get(SW1)) speed--;
        if (!gpio_get(SW2)) speed++;
        vTaskDelay(100);
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
	memset(FRAMEBUFFER, 0, sizeof(uint16_t) * WIDTH * HEIGHT);
    LCD_2IN_Clear(0xFFFF);
    // Create idle task for heartbeat
    xTaskCreate(heartbeat, "heartbeat", 128, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(drawScreen, "draw", 256, NULL, 1, NULL);
    xTaskCreate(changeSpeed, "speed", 256, NULL, 2, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){};
}
