/*
  Sweep — sweep a servo 0..180..0.

  Wire: servo signal -> a PWM-capable GPIO (any except GPIO11), VCC 5V,
  GND common. BL602's PWM hardware minimum is 2 kHz, so this library
  bit-bangs 50 Hz pulses from a background task instead.
*/
#include <Servo.h>

Servo servo;

void setup() {
  Serial.begin(115200);
  servo.attach(4);          /* servo signal on GPIO4 */
  Serial.println("sweep 0..180..0");
}

void loop() {
  for (int a = 0; a <= 180; a++) {
    servo.write(a);
    delay(10);
  }
  for (int a = 180; a >= 0; a--) {
    servo.write(a);
    delay(10);
  }
}
