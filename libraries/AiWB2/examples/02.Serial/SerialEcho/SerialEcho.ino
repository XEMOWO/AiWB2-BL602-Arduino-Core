/*
 * SerialEcho — Serial demo for the Ai-WB2-12F Arduino core.
 *
 * "Serial" lives on the SDK's console UART: TX=GPIO16, RX=GPIO7.
 * It boots at 2 Mbaud; begin(115200) re-tunes it (printf follows along,
 * exactly like Serial on ESP32). The sketch prints a banner using
 * print/println/printf, then echoes every byte you type back at you.
 *
 * Upload + monitor:
 *   tools/flash.sh <arduino_serial_echo>.bin /dev/ttyUSB0
 *   python3 tools/serial_monitor.py --port /dev/ttyUSB0 --baud 115200
 */
#include "Arduino.h"

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);

    /* Console defaults to 2 Mbaud; switch to a comfortable 115200. */
    Serial.begin(115200);

    /* Let the host terminal re-sync at the new baud before we talk. */
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
        delay(100);
    }

    Serial.println("Ai-WB2-12F SerialEcho: ready.");
    Serial.print("print(): int=");
    Serial.print(42, DEC);
    Serial.print(" hex=");
    Serial.print(0x2A, HEX);
    Serial.print(" bin=");
    Serial.print(5, BIN);
    Serial.print(" float=");
    Serial.println(3.14159, 2);
    Serial.printf("printf(): %d %#x %s\r\n", -7, 0xbeef, "ok");
}

void loop()
{
    /* Echo everything received back to the host. */
    while (Serial.available() > 0) {
        int c = Serial.read();
        if (c == '\r' || c == '\n') {
            Serial.println();
        } else {
            Serial.print((char)c);
        }
    }

    static bool ledOn = false;
    ledOn = !ledOn;
    digitalWrite(LED_BUILTIN, ledOn ? HIGH : LOW);
    delay(100);
}
