/*
 * ble_native.c — BLE peripheral for the Ai-WB2-12F (BL602), compiled as C.
 *
 * Direct translation of the SDK's applications/bluetooth/ble_slave demo:
 *   ble_controller_init() -> hci_driver_init() -> bt_enable(ready_cb)
 *   ready_cb: bt_conn_cb_register + bt_gatt_service_register + adv start
 *
 * One GATT service with two 128-bit characteristics:
 *   TX (device -> central) : NOTIFY, requires the client to enable CCC
 *   RX (central -> device) : WRITE_WITHOUT_RESP
 */
#include "ble_native.h"

#include <string.h>

#include <FreeRTOS.h>     /* configMAX_PRIORITIES */

#include <bluetooth.h>    /* bt_enable, bt_le_adv_start ...        */
#include <gatt.h>         /* BT_GATT_* macros, bt_gatt_notify     */
#include <conn.h>         /* bt_conn_cb_register                  */
#include <conn_internal.h> /* struct bt_conn                      */
#include <hci_driver.h>   /* hci_driver_init                      */
#include <ble_lib_api.h>  /* ble_controller_init                  */
#include <hci_core.h>     /* set_adv_enable (re-advertise)        */

/* ------------------------------------------------------------------ */
/* Custom 128-bit UUIDs                                                */
/* ------------------------------------------------------------------ */

#define BLE_SVC_UUID BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00000000, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb))
#define BLE_TX_UUID  BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00000000, 0x0000, 0x1001, 0x8000, 0x00805f9b34fb))
#define BLE_RX_UUID  BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0x00000000, 0x0000, 0x1002, 0x8000, 0x00805f9b34fb))

/* mutable storage so the name can be set at runtime (BT_DATA needs a
 * constant address at file scope, so this must be an array, not a pointer) */
static char s_dev_name[16] = "Ai-WB2-12F";

static struct bt_data adv_data[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
    BT_DATA(BT_DATA_NAME_COMPLETE, s_dev_name, 0), /* len filled at init */
};

static ssize_t ble_rx_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, u16_t len, u16_t offset, u8_t flags);

static struct bt_gatt_attr ble_svc_attrs[] = {
    BT_GATT_PRIMARY_SERVICE(BLE_SVC_UUID),

    /* TX characteristic (device -> central, NOTIFY) */
    BT_GATT_CHARACTERISTIC(BLE_TX_UUID, BT_GATT_CHRC_NOTIFY,
                           BT_GATT_PERM_READ, NULL, NULL, NULL),
    BT_GATT_CCC(NULL, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

    /* RX characteristic (central -> device, WRITE_WITHOUT_RESP) */
    BT_GATT_CHARACTERISTIC(BLE_RX_UUID, BT_GATT_CHRC_WRITE_WITHOUT_RESP,
                           BT_GATT_PERM_WRITE, NULL, ble_rx_write, NULL),
};

static struct bt_gatt_service ble_svc = BT_GATT_SERVICE(ble_svc_attrs);

/* the TX value attribute (index 2 = value after the char decl + service) */
#define BLE_TX_VAL_ATTR (&ble_svc_attrs[2])

/* ------------------------------------------------------------------ */
/* Connection state                                                     */
/* ------------------------------------------------------------------ */

static struct bt_conn *s_conn = NULL;
static volatile bool s_adv_started = false;
static volatile bool s_started = false;

static ble_native_conn_cb_t s_conn_cb = NULL;
static ble_native_write_cb_t s_write_cb = NULL;

static void _connected(struct bt_conn *conn, u8_t err)
{
    if (err || conn->type != BT_CONN_TYPE_LE) {
        return;
    }
    s_conn = conn;
    if (s_conn_cb) {
        s_conn_cb(true);
    }
}

static void _disconnected(struct bt_conn *conn, u8_t reason)
{
    if (conn->type != BT_CONN_TYPE_LE) {
        return;
    }
    s_conn = NULL;
    if (s_conn_cb) {
        s_conn_cb(false);
    }

    /* restart advertising so the device is findable again */
    if (s_adv_started) {
        set_adv_enable(true);
    }
}

static struct bt_conn_cb conn_callbacks = {
    .connected = _connected,
    .disconnected = _disconnected,
};

/* ------------------------------------------------------------------ */
/* GATT write callback                                                  */
/* ------------------------------------------------------------------ */

static ssize_t ble_rx_write(struct bt_conn *conn, const struct bt_gatt_attr *attr,
                            const void *buf, u16_t len, u16_t offset, u8_t flags)
{
    if (s_write_cb) {
        s_write_cb((const uint8_t *)buf, len);
    }
    return len;
}

/* ------------------------------------------------------------------ */
/* Stack bring-up                                                       */
/* ------------------------------------------------------------------ */

static void ble_ready_cb(int err)
{
    if (err) {
        return;
    }

    bt_conn_cb_register(&conn_callbacks);
    bt_gatt_service_register(&ble_svc);

    s_adv_started = (bt_le_adv_start(BT_LE_ADV_CONN, adv_data,
                                     ARRAY_SIZE(adv_data), NULL, 0) == 0);
}

bool ble_native_init(const char *name)
{
    if (s_started) {
        return true;
    }

    if (name) {
        strncpy(s_dev_name, name, sizeof(s_dev_name) - 1);
        s_dev_name[sizeof(s_dev_name) - 1] = '\0';
    }
    adv_data[1].data = (const u8_t *)s_dev_name;
    adv_data[1].data_len = (uint8_t)strlen(s_dev_name);

    ble_controller_init(configMAX_PRIORITIES - 1);
    hci_driver_init();

    if (bt_enable(ble_ready_cb) != 0) {
        return false;
    }

    s_started = true;
    return true;
}

void ble_native_deinit(void)
{
    if (!s_started) {
        return;
    }
    bt_le_adv_stop();
    s_adv_started = false;
    s_conn = NULL;
    s_started = false;
}

void ble_native_set_callbacks(ble_native_conn_cb_t conn_cb,
                              ble_native_write_cb_t write_cb)
{
    s_conn_cb = conn_cb;
    s_write_cb = write_cb;
}

int ble_native_send(const uint8_t *data, uint16_t len)
{
    uint16_t mtu;
    uint16_t offset = 0;
    int ret;

    if (!s_started || !s_conn) {
        return -1;
    }

    /* the ATT payload of a notify is mtu - 3 (1 opcode + 2 handle bytes) */
    mtu = (uint16_t)(bt_gatt_get_mtu(s_conn) - 3);
    if (mtu == 0) {
        mtu = 20; /* fallback for a not-yet-negotiated MTU */
    }

    while (len > 0) {
        uint16_t send_len = (len > mtu) ? mtu : len;
        ret = bt_gatt_notify(s_conn, BLE_TX_VAL_ATTR, data + offset, send_len);
        if (ret < 0) {
            return (offset == 0) ? ret : (int)offset;
        }
        offset += send_len;
        len -= send_len;
    }
    return (int)offset;
}

bool ble_native_connected(void)
{
    return s_conn != NULL;
}
