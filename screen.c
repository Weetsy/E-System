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
#include "hardware/i2c.h"
#include "pico/stdlib.h"
#include "myAssert.h"
#include "fonts.h"
#include "LCD_2IN.h"
#include "common.h"
#include "Pico_UPS.h"

#define MAG_SW 16 // Magnetic switch for RPM
#define WHEEL_SIZE 26 // 26 inch wheels
#define MY_WEIGHT 75 // 75kg
#define BIKE_WEIGHT 7 // 7kg
#define PIS 31415 // PI * 10,000
#define MAG_POWER 28

void drawFrameBuffer(); // Prototype

// DC and RESET should be driven high

// 153.6KB frame buffer
uint16_t FRAMEBUFFER[WIDTH * HEIGHT];
uint32_t speed;
uint32_t samples;
uint32_t energy;
uint32_t joules;
float bat;

/*
 (void) led_control powers the LED on LED_PIN when (bool) isOn is true, and
 powers it off when false.
*/
void led_control(bool isOn)
{
    isOn ? gpio_put(LED_PIN, HIGH) : gpio_put(LED_PIN, LOW);
}

void gpio_int_callback(uint gpio, uint32_t events_unused) {
    //printf("%u caused interrupt\n", gpio);
    static uint32_t checkTime;
    uint32_t currentTime = time_us_32();
    uint32_t guardTime = 65000; // 65ms (For now)
    if (checkTime + guardTime > currentTime) return;
    checkTime = currentTime; // Update checkTime
    switch (gpio) {
        case MAG_SW:
            samples++;
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
    gpio_init(MAG_SW);
    gpio_init(MAG_POWER);

    // Set up GPIO pins as output from pico
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_set_dir(DC_PIN, GPIO_OUT);
    gpio_set_dir(MAG_POWER, GPIO_OUT);

    gpio_set_dir(MAG_SW, GPIO_IN);

    gpio_pull_down(MAG_SW);

    gpio_set_function(CS_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    gpio_put(DC_PIN, HIGH);
    gpio_put(RESET_PIN, HIGH);
    gpio_put(MAG_POWER, HIGH); // Magnetic switch power rail

    gpio_set_irq_enabled_with_callback(MAG_SW, GPIO_IRQ_EDGE_RISE, true, &gpio_int_callback);
}

// Get a font bitmap for a specific character
int getFont(char c) {
	return c - 32;
}

void drawString(char *phrase, uint16_t posX, uint16_t posY, uint8_t scale, uint16_t fgColor) {
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
						if ((mask & 1<<(7-j))) FRAMEBUFFER[index] = fgColor;
					}
				}

			}
        }
        phase++;
		current = phrase[char_index++];
	}
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
    char powerString[22];

    // ps variables below used to center justify energy text
    int psLen; // Length of powerString
    int psIndex; // Index to draw the powerString to screen
    while (1) {
        memset(FRAMEBUFFER, 0, WIDTH * HEIGHT * sizeof(uint16_t));
        memset(powerString, 0, sizeof(powerString));

        // Draw speed data to screen
        sprintf(currentSpeed, "%d", speed);
        drawString("MPH", 70, 3, 3, 0xF00F);
    	drawString(currentSpeed, 0, 5, 10, 0xFFFF);

        // Draw battery data to screen
        sprintf(currentSpeed, "%.0f", bat);
        strcat(powerString, "Battery: ");
        strcat(powerString, currentSpeed);
        strcat(powerString, "%");
    	drawString(powerString, 0, 112, 2, 0x000F);
        // Clear the powerString array
        memset(powerString, 0, sizeof(powerString));

        sprintf(currentSpeed, "%.2f", energy / 1000.0);
        strcat(powerString, "Power Output: ");
        strcat(powerString, currentSpeed);
        strcat(powerString, "kW");
        drawString(powerString, 0, 100, 2, 0x0FF0);
        memset(powerString, 0, sizeof(powerString));
        drawString("kCal", 78, 30, 3, 0xAFFA);
        strcat(powerString, "   "); // Pad first 3 slots with spaces
        sprintf(&powerString[3], "%.2f", joules / 4184.0);
        // Ensure that the powerString always has 6 chars
        psLen = strlen(&powerString[3]);
        psIndex = (psLen - 7) + 3;
        if (psIndex < 0 || psIndex > 22) psIndex = 0;
        drawString(&powerString[psIndex], 104, 60, 2, 0xAFFA);
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

void getSpeed(void *notUsed) {
    static uint32_t lastPass;
    uint32_t currentTime;
    uint32_t waitTime_ms = 3000; // 3 Seconds to sample speed
    uint32_t deltaTime;
    uint32_t meters_per_sec;
    while (1) {
        // Calculate speed based on number of samples
        currentTime = time_us_32();
        deltaTime = (currentTime - lastPass) / 1000; // ms since last sample
        if (deltaTime == 0) continue;
        // 1 sample = (wheel size) * pi distance.
        // Convert the circumference to linear distance traveled
        speed = (samples * WHEEL_SIZE * PIS * 36) / (deltaTime * 12 * 528);
        meters_per_sec = (samples * WHEEL_SIZE * PIS) / (deltaTime * 394);
        energy = ((MY_WEIGHT + BIKE_WEIGHT) * (meters_per_sec * meters_per_sec)) / 2;
        joules += (energy * (waitTime_ms / 1000));
        printf("Speed is %u\n", speed);
        printf("Samples %d\n", samples);
        printf("deltaTime is %u\n", deltaTime);
        samples = 0; // Clear samples
        lastPass = currentTime; // Update last time stamp
        vTaskDelay(waitTime_ms / portTICK_PERIOD_MS);
    }
}

// Fetches battery stats using WaveShare API for the INA219 controller
void getBatteryInfo(void *notUsed) {
    UPS_init();
    while (1) {
        bat = batteryPercent();
        printf("Battery is at %f%\n", bat);
        vTaskDelay(3000);
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
    myAssert(xTaskCreate(heartbeat, "heartbeat", 128, NULL, tskIDLE_PRIORITY, NULL) == pdPASS);
    myAssert(xTaskCreate(drawScreen, "draw", 256, NULL, 1, NULL) == pdPASS);
    myAssert(xTaskCreate(getSpeed, "speed", 256, NULL, 2, NULL) == pdPASS);
    myAssert(xTaskCreate(getBatteryInfo, "bat", 256, NULL, 3, NULL) == pdPASS);
    //xTaskCreate(changeSpeed, "speed", 256, NULL, 2, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){};
}
