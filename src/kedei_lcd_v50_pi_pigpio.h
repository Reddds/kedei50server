#ifndef KEDEI_LCD_V50_PI_PIGPIO_H
#define KEDEI_LCD_V50_PI_PIGPIO_H

#define LCD_WIDTH  480
#define LCD_HEIGHT 320

#define uint8_t unsigned char
#define uint16_t unsigned int
#define uint32_t unsigned long
#define bool uint8_t
#define true 1
#define false 0

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

#endif
