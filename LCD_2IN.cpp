// Waveshare documentation and initialization functions pulled from:
// https://www.waveshare.com/wiki/2inch_LCD_Module

#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "LCD_2IN.h"
#include "common.h"

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

// Who the bleep knows what this does
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
