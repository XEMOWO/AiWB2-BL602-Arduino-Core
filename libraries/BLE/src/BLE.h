/*
 * BLE.h — BLE 5.0 GATT peripheral for the Ai-WB2-12F (BL602).
 *
 * Compact Arduino-style wrapper. The heavy lifting (controller init, GATT
 * service, advertising, callbacks) lives in the plain-C shim ble_native.c:
 * the SDK blestack GATT macros are C-only (ARRAY_SIZE relies on
 * __builtin_types_compatible_p, BT_GATT_CCC takes the address of a compound
 * literal, the service attributes use designated initializers), so they can't
 * be compiled from a C++11 translation unit. This class is a thin, type-safe
 * façade over that shim.
 *
 * Typical use:
 *   #include <BLE.h>
 *   void onWrite(const uint8_t *data, uint16_t len) { ... }
 *   void setup() {
 *     BLE.writeReceived(onWrite);
 *     BLE.begin("Ai-WB2-12F");          // returns once the stack is submitted;
 *   }                                    // advertising starts asynchronously
 *   void loop() {
 *     if (BLE.connected()) BLE.send("hello");
 *   }
 */
#ifndef BLE_h
#define BLE_h

#include <Arduino.h>
#include <stdint.h>
#include <stddef.h>

typedef void (*ble_write_cb_t)(const uint8_t *data, uint16_t len);

class BLEClass
{
public:
    BLEClass() : _started(false), _connected(false), _write_cb(NULL) {}

    /* Bring up the BLE controller + host stack and submit advertising.
     * Advertising itself starts asynchronously from the stack's ready
     * callback. Returns true when the stack was successfully submitted. */
    bool begin(const char *name = "Ai-WB2-12F");
    void end(void);

    /* true while a central is connected */
    bool connected(void) const { return _connected; }

    /* Notify the connected central via the TX characteristic.
     * Returns the number of bytes queued, or -1 if not connected. */
    int send(const uint8_t *data, uint16_t len);
    int send(const char *str) { return send((const uint8_t *)str, (uint16_t)strlen(str)); }

    /* Register a callback for incoming writes on the RX characteristic. */
    void writeReceived(ble_write_cb_t cb) { _write_cb = cb; }

private:
    bool _started;
    volatile bool _connected;
    ble_write_cb_t _write_cb;

    void _onWrite(const uint8_t *data, uint16_t len);
    void _onConn(bool connected);

    /* C-shim bridge functions, defined in BLE.cpp (ble_native callbacks). */
    friend void ble_bridge_conn(bool connected);
    friend void ble_bridge_write(const uint8_t *data, uint16_t len);
};

extern BLEClass BLE;

#endif /* BLE_h */
