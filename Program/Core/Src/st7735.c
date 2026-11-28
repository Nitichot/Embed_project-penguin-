#include "st7735.h"

#define ST7735_SWRESET 0x01
#define ST7735_SLPOUT  0x11
#define ST7735_INVOFF  0x20
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_MADCTL  0x36
#define ST7735_COLMOD  0x3A

static void ST7735_Select(uint8_t screen) {
    if (screen == 1) HAL_GPIO_WritePin(SPI3_CS1_GPIO_Port, SPI3_CS1_Pin, GPIO_PIN_RESET);
    else             HAL_GPIO_WritePin(SPI3_CS2_GPIO_Port, SPI3_CS2_Pin, GPIO_PIN_RESET);
}

static void ST7735_Unselect(uint8_t screen) {
    if (screen == 1) HAL_GPIO_WritePin(SPI3_CS1_GPIO_Port, SPI3_CS1_Pin, GPIO_PIN_SET);
    else             HAL_GPIO_WritePin(SPI3_CS2_GPIO_Port, SPI3_CS2_Pin, GPIO_PIN_SET);
}

static void ST7735_SetDC(uint8_t screen, GPIO_PinState state) {
    if (screen == 1) HAL_GPIO_WritePin(SPI3_A01_GPIO_Port, SPI3_A01_Pin, state);
    else             HAL_GPIO_WritePin(SPI3_A02_GPIO_Port, SPI3_A02_Pin, state);
}

static void ST7735_Reset(uint8_t screen) {
    if (screen == 1) {
        HAL_GPIO_WritePin(SPI3_RESET1_GPIO_Port, SPI3_RESET1_Pin, GPIO_PIN_RESET);
        HAL_Delay(20);
        HAL_GPIO_WritePin(SPI3_RESET1_GPIO_Port, SPI3_RESET1_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(SPI3_RESET2_GPIO_Port, SPI3_RESET2_Pin, GPIO_PIN_RESET);
        HAL_Delay(20);
        HAL_GPIO_WritePin(SPI3_RESET2_GPIO_Port, SPI3_RESET2_Pin, GPIO_PIN_SET);
    }
    HAL_Delay(120);
}

static void ST7735_WriteCommand(uint8_t screen, uint8_t cmd) {
    ST7735_SetDC(screen, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, 1, HAL_MAX_DELAY);
}

static void ST7735_WriteData(uint8_t screen, uint8_t* buff, size_t buff_size) {
    ST7735_SetDC(screen, GPIO_PIN_SET);
    while (buff_size > 0) {
        uint16_t chunk = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, chunk, HAL_MAX_DELAY);
        buff += chunk;
        buff_size -= chunk;
    }
}

static void ST7735_SetAddressWindow(uint8_t screen, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    ST7735_WriteCommand(screen, ST7735_CASET);
    uint8_t data[] = { 0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
    ST7735_WriteData(screen, data, sizeof(data));

    ST7735_WriteCommand(screen, ST7735_RASET);
    data[1] = y0 + ST7735_YSTART;
    data[3] = y1 + ST7735_YSTART;
    ST7735_WriteData(screen, data, sizeof(data));

    ST7735_WriteCommand(screen, ST7735_RAMWR);
}

void ST7735_Init(uint8_t screen) {
    ST7735_Select(screen);
    ST7735_Reset(screen);

    ST7735_WriteCommand(screen, ST7735_SWRESET);
    HAL_Delay(150);
    ST7735_WriteCommand(screen, ST7735_SLPOUT);
    HAL_Delay(150);

    ST7735_WriteCommand(screen, ST7735_COLMOD);
    uint8_t colmod = 0x05;
    ST7735_WriteData(screen, &colmod, 1);

    ST7735_WriteCommand(screen, ST7735_MADCTL);
    uint8_t madctl = 0xC8;
    ST7735_WriteData(screen, &madctl, 1);

    ST7735_WriteCommand(screen, ST7735_INVOFF);
    ST7735_WriteCommand(screen, ST7735_DISPON);
    HAL_Delay(100);
    ST7735_Unselect(screen);
}

void ST7735_FillRect(uint8_t screen, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if ((x >= ST7735_WIDTH) || (y >= ST7735_HEIGHT) || (w == 0) || (h == 0)) return;
    if ((x + w - 1) >= ST7735_WIDTH)  w = ST7735_WIDTH  - x;
    if ((y + h - 1) >= ST7735_HEIGHT) h = ST7735_HEIGHT - y;

    ST7735_Select(screen);
    ST7735_SetAddressWindow(screen, x, y, x + w - 1, y + h - 1);

    uint8_t line_buf[512];
    uint8_t hi = color >> 8, lo = color & 0xFF;
    for (int i = 0; i < 256; i++) {
        line_buf[i * 2] = hi;
        line_buf[i * 2 + 1] = lo;
    }

    ST7735_SetDC(screen, GPIO_PIN_SET);
    uint32_t total_pixels = (uint32_t)w * h;
    while (total_pixels > 0) {
        uint16_t chunk = (total_pixels > 256) ? 256 : total_pixels;
        HAL_SPI_Transmit(&ST7735_SPI_PORT, line_buf, chunk * 2, HAL_MAX_DELAY);
        total_pixels -= chunk;
    }
    ST7735_Unselect(screen);
}

void ST7735_FillScreen(uint8_t screen, uint16_t color) {
    ST7735_FillRect(screen, 0, 0, ST7735_WIDTH, ST7735_HEIGHT, color);
}

void ST7735_FillCircle(uint8_t screen, int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    ST7735_FillRect(screen, x0, y0 - r, 1, 2 * r + 1, color);
    int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, x = 0, y = r;
    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        ST7735_FillRect(screen, x0 + x, y0 - y, 1, 2 * y + 1, color);
        ST7735_FillRect(screen, x0 - x, y0 - y, 1, 2 * y + 1, color);
        ST7735_FillRect(screen, x0 + y, y0 - x, 1, 2 * x + 1, color);
        ST7735_FillRect(screen, x0 - y, y0 - x, 1, 2 * x + 1, color);
    }
}

void ST7735_FillRoundRect(uint8_t screen, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color) {
    ST7735_FillRect(screen, x + r, y, w - 2 * r, h, color);
    int16_t f = 1 - r, ddF_x = 1, ddF_y = -2 * r, cx = 0, cy = r;
    while (cx < cy) {
        if (f >= 0) { cy--; ddF_y += 2; f += ddF_y; }
        cx++; ddF_x += 2; f += ddF_x;
        ST7735_FillRect(screen, x + w - r - 1 + cx, y + r - cy, 1, h - 2 * r + 2 * cy, color);
        ST7735_FillRect(screen, x + w - r - 1 + cy, y + r - cx, 1, h - 2 * r + 2 * cx, color);
        ST7735_FillRect(screen, x + r - cx,         y + r - cy, 1, h - 2 * r + 2 * cy, color);
        ST7735_FillRect(screen, x + r - cy,         y + r - cx, 1, h - 2 * r + 2 * cx, color);
    }
}
