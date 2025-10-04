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

#include "i2c_fifo.h"
#include "i2c_slave.h"

#include "triangleDraw.h"
#include "Renderer.hpp"

#include <string>
#include <vector>

#define MAG_SW 16 // Magnetic switch for RPM
#define WHEEL_SIZE 26 // 26 inch wheels
#define MY_WEIGHT 75 // 75kg
#define BIKE_WEIGHT 7 // 7kg
#define PIS 31415 // PI * 10,000
#define PI 3.1415f
#define MAG_POWER 28

#define A_PIN 17
#define B_PIN 16

#define MOTOR_CNTRL_CLOCK 14
#define MOTOR_CNTRL_CUNTR 15

#define CPR 17280
#define TOTAL_ZONES 7
#define ZONES (CPR / (TOTAL_ZONES + 1))

#define ALLOWABLE_BAD_TRANSITIONS 5

void drawFrameBuffer(); // Prototype

// DC and RESET should be driven high

// 153.6KB frame buffer
uint16_t FRAMEBUFFER[WIDTH * HEIGHT];
uint32_t speed;
uint32_t samples;
uint32_t energy;
uint32_t joules;
int pos;
float bat;
int badTransitions;
int ignoredSwitches;
#define BOUNCE_TIME_US 0

typedef enum Rotation {
    Q1, Q2, Q3, Q4
} Rotation;

enum Signals {
    A_HIGH,
    A_LOW,
    B_HIGH,
    B_LOW
};
Rotation stateMachine;

// The slave implements a 256 byte memory. To write a series of bytes, the master first
// writes the memory address, followed by the data. The address is automatically incremented
// for each byte transferred, looping back to 0 upon reaching the end. Reading is done
// sequentially from the current memory address.
static struct
{
    uint8_t mem[256];
    uint8_t mem_address;
    bool mem_address_written;
} context;

/*
 (void) led_control powers the LED on LED_PIN when (bool) isOn is true, and
 powers it off when false.
*/
void led_control(bool isOn)
{
    isOn ? gpio_put(LED_PIN, HIGH) : gpio_put(LED_PIN, LOW);
}

int getZone(int position) {
    int currentPos = position % CPR;
    currentPos = currentPos / ZONES;

    if(position < 0){
        currentPos = abs(currentPos);
        currentPos = TOTAL_ZONES - currentPos;
    }

    return currentPos;
}

void handleI2Cinterrupt(i2c_inst_t *i2c, i2c_slave_event_t event){
    switch (event) {
    case I2C_SLAVE_RECEIVE: // master has written some data
        if (!context.mem_address_written) {
            // writes always start with the memory address
            context.mem_address = i2c_read_byte(i2c);
            context.mem_address_written = true;
        } else {
            // save into memory
            context.mem[context.mem_address] = i2c_read_byte(i2c);
            context.mem_address++;
        }
        break;
    case I2C_SLAVE_REQUEST: // master is requesting data
        // load from memory
        i2c_write_byte(i2c, context.mem[context.mem_address]);
        context.mem_address++;
        break;
    case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
        context.mem_address_written = false;
        break;
    default:
        break;
    }
}

void changeState(uint8_t signal) {
    static uint8_t cyc; // Number of state "cycles" so far

    // Perform different actions depending on the current state and signal
    switch (stateMachine) {
        case Q1: // If we're currently in state 1, transition to state 2 or 4
            switch (signal) {
            case B_HIGH:
                stateMachine = Q2;
                pos++;
                break;
            case A_HIGH:
                stateMachine = Q4;
                pos--;
                break;
            default:
                break;
            }
            break;
        case Q2: // If we're currently in state 2, transition to state 1 or 3
            switch (signal) {
            case B_LOW:
                stateMachine = Q1;
                pos--;
                break;
            case A_HIGH:
                stateMachine = Q3;
                pos++;
                break;
            default:
                break;
            }
            break;
        case Q3: // If we're currently in state 3, transition to state 4 or 2
            switch (signal) {
            case B_LOW:
                stateMachine = Q4;
                pos++;
                break;
            case A_LOW:
                stateMachine = Q2;
                pos--;
                break;
            }
            break;
        case Q4: // If we're currently in state 4, transition to state 1 or 3
            switch (signal) {
            case A_LOW: 
                stateMachine = Q1;
                pos++;
                break;
            case B_HIGH:
                stateMachine = Q3;
                pos--;
                break;
            default:
                break;
            }
            break;
        default:
            //printf("Transition error detected\n");
            myAssert(badTransitions++ < ALLOWABLE_BAD_TRANSITIONS);
            break;
    }
}

void gpio_int_callback(uint gpio, uint32_t events_unused) {
    //printf("%u caused interrupt\n", gpio);
    static uint32_t checkTime;
    uint32_t currentTime = time_us_32();
    if (checkTime + BOUNCE_TIME_US > currentTime) {
        ignoredSwitches++;
        return;
    }
    checkTime = currentTime; // Update checkTime
    switch (gpio) {
        case A_PIN:
            changeState(events_unused == GPIO_IRQ_EDGE_RISE ? A_HIGH : A_LOW);
            break;
        case B_PIN:
            changeState(events_unused == GPIO_IRQ_EDGE_RISE ? B_HIGH : B_LOW);
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

    // Initialize Motor Control GPIOs
    gpio_init(MOTOR_CNTRL_CLOCK);
    gpio_init(MOTOR_CNTRL_CUNTR);

    // Initialize motor phase pins
    gpio_init(A_PIN);
    gpio_init(B_PIN);

    // Set up GPIO pins as output from pico
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_set_dir(RESET_PIN, GPIO_OUT);
    gpio_set_dir(DC_PIN, GPIO_OUT);
    gpio_set_dir(MAG_POWER, GPIO_OUT);

    gpio_set_dir(MAG_SW, GPIO_IN);
    gpio_set_dir(A_PIN, GPIO_IN);
    gpio_set_dir(B_PIN, GPIO_IN);

    gpio_set_dir(MOTOR_CNTRL_CLOCK, GPIO_OUT);
    gpio_set_dir(MOTOR_CNTRL_CUNTR, GPIO_OUT);

    gpio_pull_down(MAG_SW);

    gpio_set_function(CS_PIN, GPIO_FUNC_SPI);
    gpio_set_function(CLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MOSI_PIN, GPIO_FUNC_SPI);
    gpio_set_function(MISO_PIN, GPIO_FUNC_SPI);

    gpio_set_function(0, GPIO_FUNC_I2C);
    gpio_set_function(1, GPIO_FUNC_I2C);

    i2c_slave_init(i2c0, 0x10, handleI2Cinterrupt);

    gpio_put(DC_PIN, HIGH);
    gpio_put(RESET_PIN, HIGH);
    gpio_put(MAG_POWER, HIGH); // Magnetic switch power rail

    gpio_set_irq_enabled_with_callback(MAG_SW, GPIO_IRQ_EDGE_RISE, true, &gpio_int_callback);
    gpio_set_irq_enabled_with_callback(A_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);
    gpio_set_irq_enabled_with_callback(B_PIN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_int_callback);
}

// Get a font bitmap for a specific character
int getFont(char c) {
	return c - 32;
}

void drawString(const char *phrase, uint16_t posX, uint16_t posY, uint8_t scale, uint16_t fgColor) {
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
        context.mem[0x17]++;
        context.mem[0x22] = getZone(pos);
        printf("0x17 Value: %x Encoder Pos: %d Zone: %d\n", context.mem[0x17], pos, getZone(pos)); // 1Hz blinking
        // Blink for 1Hz
        if(context.mem[0x24] == 0){
            gpio_put(MOTOR_CNTRL_CUNTR, LOW);
            gpio_put(MOTOR_CNTRL_CLOCK, LOW);
        } else if(context.mem[0x24] % 2){
            gpio_put(MOTOR_CNTRL_CUNTR, HIGH);
            gpio_put(MOTOR_CNTRL_CLOCK, LOW);
        } else {
            gpio_put(MOTOR_CNTRL_CUNTR, LOW);
            gpio_put(MOTOR_CNTRL_CLOCK, HIGH);
        }
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
    // Create a sample IRREGULAR triangle
    auto v1 = Vertex(0, 50, 0);
    auto v2 = Vertex(70, 0, 0);
    auto v3 = Vertex(132, 99, 0);
    std::vector<Vertex> vertices;
    vertices.push_back(v1);
    vertices.push_back(v2);
    vertices.push_back(v3);
    auto v4 = Vertex(0, 150, 0);
    auto v5 = Vertex(50, 100, 0);
    auto v6 = Vertex(150, 150, 0);
    std::vector<Vertex> verticesToo;
    verticesToo.push_back(v4);
    verticesToo.push_back(v5);
    verticesToo.push_back(v6);
    auto v7 = Vertex(-100, 300, 0);
    auto v8 = Vertex(50, 250, 0);
    auto v9 = Vertex(150, 250, 0);
    std::vector<Vertex> verticesTree;
    verticesTree.push_back(v7);
    verticesTree.push_back(v8);
    verticesTree.push_back(v9);
    auto tri = Triangle(vertices);
    auto tri2 = Triangle(verticesToo);
    auto tri3 = Triangle(verticesTree);
    Renderer renderer(FRAMEBUFFER, WIDTH, HEIGHT);
    uint16_t color = 0xFFFE;
    int direction = false;
    auto samples = 10;
    auto fps = 0.0f;
    std::vector<uint32_t>timings;
    while (1) {
        memset(FRAMEBUFFER, 0, WIDTH * HEIGHT * sizeof(uint16_t));
        if (color + 1 >= 0xFFFF) {
            direction = false;
        }
        if (color - 1 <= 0x0000) {
            direction = true;
        }
        if (direction) color++;
        else color--;
        auto before = time_us_32();
        //renderer.drawTriangle(tri, color);
        //renderer.drawTriangle(tri2, color);
        //renderer.drawTriangle(tri3, color);
        auto after = time_us_32();
        if (timings.size() > samples) {
            // Get average
            auto sum = 0;
            for (auto time : timings) { sum += time; }
            auto average_spf = static_cast<float>(sum) / static_cast<float>(timings.size() * 1000000);
            fps = 1.0f / average_spf;
            timings.clear();
        }
        drawString(std::string("FPS: " + std::to_string(static_cast<int>(fps))).c_str(), 0, 110, 2, 0xFFFF);
        drawString(std::string("POS:  " + std::to_string(pos)).c_str(), 0, 300, 3, 0xFFFF);
        drawString(std::string("ZONE: " + std::to_string(getZone(pos))).c_str(), 0, 20, 3, 0xFFFF);
        drawString("---debug info---", 0, 100, 1, 0xFFFF);
        drawString(std::string("badTransitions: " + std::to_string(badTransitions)).c_str(), 0, 108, 1, 0xFFFF);
        drawString(std::string("ignoredSwitches: " + std::to_string(ignoredSwitches)).c_str(), 0, 116, 1, 0xFFFF);
        drawString(std::string("guard time: " + std::to_string(BOUNCE_TIME_US)).c_str(), 0, 124, 1, 0xFFFF);
        timings.push_back(after - before);
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
        SPI_CPOL_0,
        SPI_CPHA_0,
        SPI_MSB_FIRST
    );

    LCD_2IN_Init();
	memset(FRAMEBUFFER, 0, sizeof(uint16_t) * WIDTH * HEIGHT);
    LCD_2IN_Clear(0xFFFF);
    // Create idle task for heartbeat
    myAssert(xTaskCreate(heartbeat, "heartbeat", 128, NULL, tskIDLE_PRIORITY, NULL) == pdPASS);
    myAssert(xTaskCreate(drawScreen, "draw", 2048, NULL, 1, NULL) == pdPASS);
    //myAssert(xTaskCreate(getSpeed, "speed", 256, NULL, 2, NULL) == pdPASS);
    //myAssert(xTaskCreate(getBatteryInfo, "bat", 256, NULL, 3, NULL) == pdPASS);
    //xTaskCreate(changeSpeed, "speed", 256, NULL, 2, NULL);
    // Start task scheduler to start all above tasks
    vTaskStartScheduler();

    while(1){

    };
}
