/*
  SerialBanner — verify the Ai-WB2-12F Arduino core over UART.

  The SDK's log console prints on UART0 (TX=GPIO16 / RX=GPIO7) at 2 Mbaud.
  Open a serial monitor at 2000000 baud and you should see this banner
  repeating, interleaved with the LED blink:
      Ai-WB2-12F Arduino core: setup()/loop() running

  printf() is routed to that console by the SDK, so no Serial object is
  needed yet (a proper Serial class is on the roadmap).
*/

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  printf("Ai-WB2-12F Arduino core: setup() done, entering loop()\r\n");
}

void loop() {
  printf("Ai-WB2-12F Arduino core: setup()/loop() running\r\n");
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
