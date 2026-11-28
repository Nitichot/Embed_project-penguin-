#ifndef __ST7735_H
#define __ST7735_H

#include "main.h"
#include <stdlib.h>

extern SPI_HandleTypeDef hspi3;
#define ST7735_SPI_PORT hspi3

#define ST7735_WIDTH  128
#define ST7735_HEIGHT 128
#define ST7735_XSTART 2
#define ST7735_YSTART 3

#define ST7735_BLACK   0x0000
#define ST7735_BLUE    0x001F
#define ST7735_RED     0xF800
#define ST7735_GREEN   0x07E0
#define ST7735_CYAN    0x07FF
#define ST7735_MAGENTA 0xF81F
#define ST7735_YELLOW  0xFFE0
#define ST7735_WHITE   0xFFFF

/* ระบุ screen = 1 (จอซ้าย) หรือ screen = 2 (จอขวา) */
void ST7735_Init(uint8_t screen);
void ST7735_FillRect(uint8_t screen, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7735_FillScreen(uint8_t screen, uint16_t color);
void ST7735_FillCircle(uint8_t screen, int16_t x0, int16_t y0, int16_t r, uint16_t color);
void ST7735_FillRoundRect(uint8_t screen, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);

#endif
