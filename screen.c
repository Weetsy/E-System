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
#define MAG_SW 16 // Magnetic switch for RPM
#define WHEEL_SIZE 26 // 26 inch wheels
#define PI 3.14159265359

void drawFrameBuffer(); // Prototype

// DC and RESET should be driven high

// 153.6KB frame buffer
uint16_t FRAMEBUFFER[WIDTH * HEIGHT];
SemaphoreHandle_t sem;
int speed;

/*
 (void) led_control powers the LED on LED_PIN when (bool) isOn is true, and
 powers it off when false.
*/
void led_control(bool isOn)
{
    isOn ? gpio_put(LED_PIN, HIGH) : gpio_put(LED_PIN, LOW);
}

void gpio_int_callback(uint gpio, uint32_t events_unused) {
    static uint32_t checkTime;
    uint32_t currentTime = time_us_32();
    uint32_t guardTime = 65000; // 65000ms (For now)
    if (checkTime + guardTime > currentTime) return;
    checkTime = currentTime; // Update checkTime
    BaseType_t res;
    switch (gpio) {
        case SW1:
            speed++;
            break;
        case SW2:
            speed--;
            break;
        case MAG_SW:
            // Send magnetic switch signal to queue
            res = xSemaphoreGiveFromISR(sem, NULL);
            if (res == errQUEUE_FULL) {
                printf("Error: Semaphore give error\n");
            }
            printf("Giving semaphore\n");
            break;
        default:
            break;
    }
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
    gpio_init(MAG_SW);

    // Set up GPIO pins as output from pico
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_set_dir(DC_PIN, GPIO_OUT);

    gpio_set_dir(SW1, GPIO_IN);
    gpio_set_dir(SW2, GPIO_IN);
    gpio_set_dir(MAG_SW, GPIO_IN);

    gpio_pull_up(SW1);
    gpio_pull_up(SW2);

    gpio_set_function(CS_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    gpio_put(DC_PIN, HIGH);
    gpio_put(RESET_PIN, HIGH);

    gpio_set_irq_enabled_with_callback(SW1, GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);
    gpio_set_irq_enabled_with_callback(SW2, GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);
    gpio_set_irq_enabled_with_callback(MAG_SW, GPIO_IRQ_EDGE_RISE, true, &gpio_int_callback);
}

// Get a font bitmap for a specific character
int getFont(char c) {
	return c - 32;
}

void drawString(char *phrase, uint16_t posX, uint16_t posY, uint8_t scale, uint16_t fgColor, uint16_t bgColor) {
    // Composite a string onto the frame buffer in a non-destructive manner
    // Assume the 8 by 8 pixel format
    int char_index = 0;
    if (phrase == NULL) return;
    char current = phrase[char_index++];
    uint8_t mask;
    uint8_t phase = 0;
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
                        // Band-aid solution
                        if (index < 0 || index >= WIDTH * HEIGHT) {
                            continue;
                        }
						FRAMEBUFFER[index] = (mask & 1<<(7-j)) ? fgColor : bgColor;
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
        drawString("MPH", 70, 3, 3, 0xF00F, 0);
    	drawString(currentSpeed, 0, 5, 10, 0xFFFF, 0);
    	drawString("Battery: 71%", 0, 112, 2, 0x000F, 0);
        drawString("Power Output: 120W", 0, 100, 2, 0x0FF0, 0);
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

void getSpeed(void *notUsed) {
    static uint32_t lastPass;
    uint32_t currentTime;
    while (1) {
        // Receive magnetic switch signals from queue
        xSemaphoreTake(sem, portMAX_DELAY);
        printf("Received semaphore\n");
        currentTime = time_us_32();
        // Calculate RPM based on lastPass to now
        double deltaTime = (currentTime - lastPass) / (1000.0 * 1000.0); // In seconds
        // We did one revolution whenever this function runs
        double distance = (PI * WHEEL_SIZE) / (12.0 * 5280.0); // Distance in miles
        // Convert time to hours
        double hours = deltaTime / 3600.0;
        speed = (uint32_t) (distance / hours);
        lastPass = currentTime;
    }
}

int main()
{
    // Initialize stdio
    stdio_init_all();
    sleep_ms(4000);
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

    sem = xSemaphoreCreateCounting(20, 5);
    myAssert(sem != NULL);
    // Create idle task for heartbeat
    xTaskCreate(heartbeat, "heartbeat", 128, NULL, tskIDLE_PRIORITY, NULL);
    xTaskCreate(drawScreen, "draw", 200, NULL, 1, NULL);
    xTaskCreate(getSpeed, "speed", 128, NULL, 2, NULL);
    //xTaskCreate(changeSpeed, "speed", 256, NULL, 2, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){};
}
