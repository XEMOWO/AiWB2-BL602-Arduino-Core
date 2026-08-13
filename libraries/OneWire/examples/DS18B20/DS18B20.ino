/*
  DS18B20 — read temperature over 1-Wire on GPIO4.

  Wire: DS18B20 VCC->3.3V, GND->GND, DQ->GPIO4, plus a 4.7k pull-up from
  DQ to 3.3V (the internal pull-up is marginal for long leads).
*/
#include <OneWire.h>

OneWire ow(4);

void setup() {
  Serial.begin(115200);
  delay(300);
  if (!ow.reset()) {
    Serial.println("no DS18B20 on GPIO4");
    return;
  }
  ow.skip();
  ow.write(0x44);                /* start conversion */
}

void loop() {
  delay(1000);

  if (!ow.reset()) return;
  ow.skip();
  ow.write(0xBE);                /* read scratchpad */
  uint8_t sp[9];
  ow.read_bytes(sp, 9);
  if (OneWire::crc8(sp, 8) != sp[8]) {
    Serial.println("CRC error");
    return;
  }
  int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
  Serial.printf("temp = %.2f C\n", (double)(raw / 16.0f));

  ow.reset();
  ow.skip();
  ow.write(0x44);                /* next conversion */
}
