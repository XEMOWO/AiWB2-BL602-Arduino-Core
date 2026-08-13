/*
 * SeeedTouchScreen.h - Seeed TFT Touch Shield V2 touch driver (BL602 shim).
 *
 * Compile-compatible no-op. The touch controller is never read on the WB2;
 * getPoint() always returns (0,0,0) so the paint example's pressure test
 * (`p.z > __PRESURE`) simply never paints. Pins/macros keep the Seeed API.
 */

#ifndef __SEED_TOUCH_SCREEN_H__
#define __SEED_TOUCH_SCREEN_H__

#include <Arduino.h>

/* touch plate drive pins (Seeed shield wiring; not wired on WB2) */
#define XP 0
#define YP 1
#define XM 2
#define YM 3

#define __PRESURE 200

/* raw ADC limits read back from the touch panel (Seeed defaults) */
#define TS_MINX 150
#define TS_MAXX 915
#define TS_MINY 150
#define TS_MAXY 890

struct Point
{
    int x;
    int y;
    int z;
};

class TouchScreen
{
public:
    TouchScreen(uint8_t xp, uint8_t yp, uint8_t xm, uint8_t ym)
        : _xp(xp), _yp(yp), _xm(xm), _ym(ym) {}

    Point getPoint()
    {
        (void)_xp; (void)_yp; (void)_xm; (void)_ym;
        Point p = { 0, 0, 0 };
        return p;
    }

    bool isPressed() { return false; }

private:
    uint8_t _xp, _yp, _xm, _ym;
};

#endif /* __SEED_TOUCH_SCREEN_H__ */
