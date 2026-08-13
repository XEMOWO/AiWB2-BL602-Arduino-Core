/*
 * pins_arduino.h — Ai-WB2-12F (BL602) pin mapping.
 *
 * Pin numbers equal BL602 GPIO numbers (identity map), so the tables below
 * are 1:1.  Board-level availability comes from the Ai-WB2-12F 规格书 V1.1.3
 * (管脚定义表, §4):
 *
 *   推荐可用  (9):  IO3 IO4 IO5 IO7(RXD) IO11 IO12 IO14 IO16(TXD) IO17
 *   Flash 共用(6):  IO0 IO1 IO2 IO20 IO21 IO22   与内部 Flash 复用,不推荐使用
 *   Bootstrap (1):  IO8                        默认 NC;上电瞬间为高电平进烧录模式
 *   未引出    (7):  IO6 IO9 IO10 IO13 IO15 IO18 IO19
 *
 * IO7/IO16 是 UART0 = Serial,外设示例应避开。数字引脚 0..21 恒等映射,对
 * 未引出的引脚操作是安全的 no-op(该 GPIO 在硅片上存在,只是板上没有焊盘)。
 *
 * ADC:板级只引出 5 个可做模拟输入的脚。analog_pin_to_gpio 按 SDK 通道序
 *      (ch0/ch1/ch2/ch4/ch10) 排列,analogRead 也接受裸 GPIO 号。
 * PWM:通道 = GPIO % 5;IO11 无 PWM 功能;IO7/16 是 Serial 不能用。
 *
 * I2C / SPI 默认脚与 SDK 官方 demo 一致,全部落在"推荐可用"组:
 *    I2C SCL=12 SDA=3    SPI SCK=3 MOSI=12 MISO=17 CS=4
 * 注意 I2C(12/3) 与 SPI(3/12) 默认脚有重叠,不能同时用默认值;同时用需显式
 * 指定第二组(如 SPI SCK=11 MOSI=4 MISO=17 CS=14)。
 */
#ifndef Pins_Arduino_h
#define Pins_Arduino_h

#include <stdint.h>

#define NUM_DIGITAL_PINS   22
#define NUM_ANALOG_INPUTS  5

/* Arduino pin number -> BL602 GPIO number (1:1 for now). */
extern const uint8_t digital_pin_to_gpio[NUM_DIGITAL_PINS];

/* GPIOs usable as SAR ADC inputs. Only these 5 are broken out on the module:
 *   A0=GPIO12(ch0)  A1=GPIO4(ch1)  A2=GPIO14(ch2)  A3=GPIO5(ch4)  A4=GPIO11(ch10) */
extern const uint8_t analog_pin_to_gpio[NUM_ANALOG_INPUTS];

/* Onboard LED. The SDK "evb" demo drives the LED on GPIO14 (PWM CH4).
 * NOTE: GPIO4 is on the same PWM channel (CH4) as the LED. */
#define LED_BUILTIN 14

/* Analog pins live past the digital range (22..26). */
#define A0 22
#define A1 23
#define A2 24
#define A3 25
#define A4 26

/* I2C default pins (Wire.begin()) — SDK demo, both in the recommended set. */
#define PIN_WIRE_SDA  3
#define PIN_WIRE_SCL  12

/* SPI default pins (SPI.begin()) — SDK demo, all in the recommended set. */
#define PIN_SPI_SS    4
#define PIN_SPI_SCK   3
#define PIN_SPI_MOSI  12
#define PIN_SPI_MISO  17

/* Friendly aliases. These are `static const`, NOT macros — as in the ESP8266
 * core — so that library constructors whose parameters are named MOSI/SCLK/SS
 * (e.g. Adafruit_ST7796S) can still shadow them. A #define would mangle the
 * parameter into a numeric literal. */
static const uint8_t SDA  = PIN_WIRE_SDA;
static const uint8_t SCL  = PIN_WIRE_SCL;
static const uint8_t SS   = PIN_SPI_SS;
static const uint8_t SCK  = PIN_SPI_SCK;
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;

/* Serial1 (UART1) default pins — both in the recommended set. UART1 on BL602
 * can route to several GPIOs; pass explicit pins to Serial1.begin() if these
 * clash with another peripheral. */
#define PIN_SERIAL1_TX  11
#define PIN_SERIAL1_RX  17

/* Serial (UART0) default pins — the SDK console port. Named like the ESP8266
 * variant so the compat layer (HardwareSerial begin overloads) can reference
 * them; ESP8266 sketches that hard-code TX=GPIO1/RX=GPIO3 are remapped here. */
#define PIN_SERIAL0_TX  16
#define PIN_SERIAL0_RX  7

/* Every digital pin number is its own interrupt number (identity map).
 * EXTERNAL_NUM_INTERRUPTS is the ESP8266 name for the count; libraries such as
 * PJRC's Encoder check it (CORE_NUM_INTERRUPT = EXTERNAL_NUM_INTERRUPTS). */
#define EXTERNAL_NUM_INTERRUPTS NUM_DIGITAL_PINS
#define digitalPinToInterrupt(p) (p)

/* Flash-shared pins (spec §4: IO0/1/2/20/21/22 multiplex the internal flash;
 * GPIO22 additionally hosts the SPI-XIP bus — driving it freezes the CPU).
 * ESP8266's variant headers expose isFlashInterfacePin() so bit-bang code
 * (SoftwareSerial) keeps off those pins; same intent here. */
#define isFlashInterfacePin(p) (((p) >= 0 && (p) <= 2) || ((p) >= 20 && (p) <= 22))

/* Bit mask helpers (AVR-style names, used by bit-bang libraries). Because pin
 * numbers equal GPIO numbers, the mask is just the GPIO bit in the GLB GPIO
 * value registers. digitalPinToPort() is unused on BL602 (no port structure). */
#define digitalPinToBitMask(p) (1UL << digital_pin_to_gpio[(p)])
#define digitalPinToPort(p)    (0)

/* Port registers (ESP8266/AVR style). BL602 has one 32-bit GPIO bank in the
 * GLB block, so every "port" is the same register: OUTPUT@0x188, INPUT@0x180.
 * `*portOutputRegister(digitalPinToPort(p)) |= digitalPinToBitMask(p)` sets the
 * right GLB bit, matching the bit-bang pattern (e.g. Adafruit_BusIO SW SPI). */
#define portOutputRegister(port) ((volatile uint32_t *)0x40000188UL)
#define portInputRegister(port)  ((volatile uint32_t *)0x40000180UL)

#endif /* Pins_Arduino_h */
