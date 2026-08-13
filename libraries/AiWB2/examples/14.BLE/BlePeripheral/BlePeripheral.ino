/*
  BlePeripheral — advertise the Ai-WB2-12F as a BLE 5.0 GATT peripheral.

  Scan with a phone app (e.g. nRF Connect / 安信可 AT 工具). The device
  advertises as "Ai-WB2-12F" with one service:

    0x0000-1000-8000-00805f9b34fb
      TX  (0x1001)  NOTIFY — every second a counter is pushed while connected
      RX  (0x1002)  WRITE_WITHOUT_RESP — bytes you send are echoed back

  Open the Serial Monitor at 115200 to watch connect/disconnect and traffic.

  GATT service is not encrypted/pairing-protected; this is a plain demo.
*/
#include <Arduino.h>
#include <BLE.h>

void onWrite(const uint8_t *data, uint16_t len);

static uint32_t counter = 0;

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("BLE peripheral demo (Ai-WB2-12F)");

  BLE.writeReceived(onWrite);

  if (!BLE.begin("Ai-WB2-12F")) {
    Serial.println("BLE begin() failed");
    while (1) { delay(1000); }
  }
  Serial.println("Advertising as Ai-WB2-12F ...");
}

void loop() {
  if (BLE.connected()) {
    /* push one notify per second while a central is connected */
    char buf[32];
    int n = snprintf(buf, sizeof(buf), "tick %lu", (unsigned long)(counter++));
    int ret = BLE.send((const uint8_t *)buf, (uint16_t)n);
    if (ret < 0) {
      Serial.println("send() failed (not connected?)");
    }
  }
  delay(1000);
}

/* called on the BLE stack thread when the central writes the RX characteristic */
void onWrite(const uint8_t *data, uint16_t len) {
  Serial.print("RX[");
  Serial.print(len);
  Serial.print("]: ");
  for (uint16_t i = 0; i < len; i++) {
    Serial.print((char)data[i]);
  }
  Serial.println();

  /* echo it back via the notify characteristic */
  BLE.send(data, len);
}
