/*
  IRreceive — decode NEC frames from a TSOP38 receiver.

  Wire: TSOP38 out -> GPIO5, VCC -> 3.3V, GND -> GND. Point a remote at it;
  decoded codes print in the Serial monitor. IRResults.value is the 32-bit
  NEC code, IRResults.repeat flags auto-repeat frames.
*/
#include <IRremote.h>

IRrecv  irrecv(5);
IRResults res;

void setup() {
  Serial.begin(115200);
  irrecv.enableIRIn();
  Serial.println("point a NEC remote at the TSOP");
}

void loop() {
  if (irrecv.decode(&res)) {
    Serial.printf("IR: 0x%08lX repeat=%d\n",
                  (unsigned long)res.value, (int)res.repeat);
    irrecv.resume();
  }
}
