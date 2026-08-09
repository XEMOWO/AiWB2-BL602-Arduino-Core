/*
 * ble_native.h — C bridge between the Arduino BLE library and the SDK's
 * blestack. The SDK GATT macros (BT_GATT_*, ARRAY_SIZE, BT_LE_ADV_CONN)
 * are C-only, so all direct blestack use lives here in a plain C file;
 * BLE.cpp just calls these functions.
 */
#ifndef BLE_NATIVE_H
#define BLE_NATIVE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Callbacks the C shim fires. */
typedef void (*ble_native_conn_cb_t)(bool connected);
typedef void (*ble_native_write_cb_t)(const uint8_t *data, uint16_t len);

/* Start the controller + host stack and begin advertising.
 * Returns true if the stack was submitted (advertising begins asynchronously
 * from the bt_enable ready callback). */
bool ble_native_init(const char *name);

/* Stop advertising and shut down the stack. */
void ble_native_deinit(void);

/* Register the callbacks the Arduino layer routes to. */
void ble_native_set_callbacks(ble_native_conn_cb_t conn_cb,
                              ble_native_write_cb_t write_cb);

/* Notify the connected central. Returns bytes queued or -1 if not ready. */
int ble_native_send(const uint8_t *data, uint16_t len);

/* True while a central is connected. */
bool ble_native_connected(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_NATIVE_H */
