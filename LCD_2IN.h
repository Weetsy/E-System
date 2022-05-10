// Waveshare documentation and initialization functions pulled from:
// https://www.waveshare.com/wiki/2inch_LCD_Module
#ifndef LCD_2IN_H
#define LCD_2IN_H

#define CS_PIN   5  /* GPIO5 */
#define CLK_PIN  2  /* GPIO2 */
#define MOSI_PIN 3  /* GPIO3 */
#define MISO_PIN 4  /* GPIO4 */

#define LED_PIN 25 // Pico's built in LED
#define DC_PIN 6 // Extra DC pin for display
#define RESET_PIN 7 // Drive this pin LOW to reset display
#define BACKLIGHT 8 // Backlight pin

static void LCD_2IN_Reset(void);
void LCD_2IN_Write_Command(uint8_t address);
void LCD_2IN_WriteData_Byte(uint8_t byte);
void LCD_2IN_Init(void);
void LCD_2IN_SetWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t  Yend);
void DEV_SPI_Write_nByte(uint8_t *data, uint32_t length);
void LCD_2IN_Clear(uint16_t Color);
#endif
