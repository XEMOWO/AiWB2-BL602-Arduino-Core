/*
 * BLE.cpp — thin C++ wrapper over the C shim in ble_native.c.
 *
 * The SDK blestack GATT/service/advertising macros are C-only (see BLE.h for
 * the reasons), so all direct blestack use lives in ble_native.c. This file
 * only bridges the shim's callbacks onto the BLEClass singleton and forwards
 * the public API calls.
 */
#include "BLE.h"

#include <string.h>

#include "ble_native.h"

BLEClass BLE;

/* Routed to BLEClass by ble_native_set_callbacks(). */
void ble_bridge_conn(bool connected)
{
    BLE._onConn(connected);
}

void ble_bridge_write(const uint8_t *data, uint16_t len)
{
    BLE._onWrite(data, len);
}

void BLEClass::_onWrite(const uint8_t *data, uint16_t len)
{
    if (_write_cb) {
        _write_cb(data, len);
    }
}

void BLEClass::_onConn(bool connected)
{
    _connected = connected;
}

bool BLEClass::begin(const char *name)
{
    if (_started) {
        return true;
    }

    /* Register the bridges before init so no connect/write can be missed. */
    ble_native_set_callbacks(ble_bridge_conn, ble_bridge_write);

    if (!ble_native_init(name)) {
        return false;
    }

    _started = true;
    return true;
}

void BLEClass::end(void)
{
    if (!_started) {
        return;
    }
    ble_native_deinit();
    _started = false;
    _connected = false;
}

int BLEClass::send(const uint8_t *data, uint16_t len)
{
    return ble_native_send(data, len);
}
