/*
 * TFTv2.h - Seeed TFT Touch Shield V2 for the Ai-WB2-12F (BL602).
 *
 * Compile-compatible port. The WB2 board has no such display; every drawing
 * call is a benign no-op so the Seeed examples (drawCircle/drawLine/drawNumber/
 * text/paint/tftbmp...) compile unchanged and run without crashing. The global
 * `Tft` object and the TFT_BL_ON / TFT_CS_* / TFT_DC_* macros keep the exact
 * API surface of the Seeed library.
 */

#ifndef __TFTV2_H__
#define __TFTV2_H__

#include <Arduino.h>

/* ---- backlight / chip-select / data-command helpers (no-ops here) ---- */
#define TFT_BL_ON      ((void)0)
#define TFT_BL_OFF     ((void)0)
#define TFT_CS_HIGH    ((void)0)
#define TFT_CS_LOW     ((void)0)
#define TFT_DC_HIGH    ((void)0)
#define TFT_DC_LOW     ((void)0)
#define TFT_WR_HIGH    ((void)0)
#define TFT_WR_LOW     ((void)0)
#define TFT_RD_HIGH    ((void)0)
#define TFT_RD_LOW     ((void)0)

/* ---- 16-bit RGB565 color constants (Seeed TFT library) ---- */
#define BLACK     0x0000
#define BLUE      0x001F
#define RED       0xF800
#define GREEN     0x07E0
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define YELLOW    0xFFE0
#define WHITE     0xFFFF
#define GRAY1     0x8410
#define GRAY2     0x4208
#define NAVY      0x000F
#define DARKCYAN  0x03EF
#define MAROON    0x7800
#define PURPLE    0x780F
#define OLIVE     0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY  0x7BEF
#define ORANGE    0xFD20
#define GREENYELLOW 0xAFE5
#define PINK      0xF81F

class TFTv2
{
public:
    void TFTinit() {}

    void drawCircle(int poX, int poY, int r, uint16_t color) { (void)poX; (void)poY; (void)r; (void)color; }
    void fillCircle(int poX, int poY, int r, uint16_t color) { (void)poX; (void)poY; (void)r; (void)color; }

    void drawLine(unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, unsigned int color) { (void)x0; (void)y0; (void)x1; (void)y1; (void)color; }
    void drawVerticalLine(unsigned int poX, unsigned int poY, unsigned int length, unsigned int color) { (void)poX; (void)poY; (void)length; (void)color; }
    void drawHorizontalLine(unsigned int poX, unsigned int poY, unsigned int length, unsigned int color) { (void)poX; (void)poY; (void)length; (void)color; }

    void drawNumber(long long_num, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) { (void)long_num; (void)poX; (void)poY; (void)size; (void)fgcolor; }
    void drawFloat(float floatNumber, uint8_t decimal, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) { (void)floatNumber; (void)decimal; (void)poX; (void)poY; (void)size; (void)fgcolor; }
    void drawFloat(float floatNumber, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) { (void)floatNumber; (void)poX; (void)poY; (void)size; (void)fgcolor; }

    void drawChar(char ascii, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) { (void)ascii; (void)poX; (void)poY; (void)size; (void)fgcolor; }
    void drawString(const char *string, uint16_t poX, uint16_t poY, uint16_t size, uint16_t fgcolor) { (void)string; (void)poX; (void)poY; (void)size; (void)fgcolor; }

    void fillScreen(uint16_t XL, uint16_t XR, uint16_t YU, uint16_t YD, uint16_t color) { (void)XL; (void)XR; (void)YU; (void)YD; (void)color; }
    void fillRectangle(uint16_t poX, uint16_t poY, uint16_t length, uint16_t width, uint16_t color) { (void)poX; (void)poY; (void)length; (void)width; (void)color; }
    void drawRectangle(uint16_t poX, uint16_t poY, uint16_t length, uint16_t width, uint16_t color) { (void)poX; (void)poY; (void)length; (void)width; (void)color; }

    void setCol(int start, int end) { (void)start; (void)end; }
    void setPage(int start, int end) { (void)start; (void)end; }
    void sendCMD(unsigned char cmd) { (void)cmd; }
};

extern TFTv2 Tft;

#endif /* __TFTV2_H__ */
