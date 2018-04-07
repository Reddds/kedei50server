#ifndef KEDEI_LCD_V50_PI_PIGPIO_H
#define KEDEI_LCD_V50_PI_PIGPIO_H

#include <stdint.h>
#include <stdbool.h>

#define LCD_WIDTH  480
#define LCD_HEIGHT 320

//#define uint8_t unsigned char
//#define uint16_t unsigned int
//#define uint32_t unsigned long
//#define bool uint8_t
//#define true 1
//#define false 0

#define READ_32(buf, offset) (buf[offset] + (buf[offset + 1] << 8) + (buf[offset + 2] << 16) + (buf[offset + 3] << 24))
#define READ_16(buf, offset) (buf[offset] + (buf[offset + 1] << 8))

enum lcd_rotations
{
	LCD_ROTATE_0,
	LCD_ROTATE_90,
	LCD_ROTATE_180,
	LCD_ROTATE_270,
};

int lcd_open(void);
int lcd_close(void);
void lcd_img(char *fname, uint16_t x, uint16_t y);
void lcd_fill(uint16_t col);
void lcd_init(enum lcd_rotations rotation);
void lcd_colorRGB(uint8_t r, uint8_t g, uint8_t b);
void lcd_setframe(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void create_sensor_thread();


#endif
