/*
 * espnow.cpp — runtime-harmless ESP-NOW stub for the BL602 (see espnow.h).
 *
 * The BL602 SDK has no ESP-NOW PHY. Every call reports success (0) so
 * ESP8266 mesh code that checks return codes keeps running; peers are
 * treated as existing (is_peer_exist → 1) so encrypted-send asserts pass;
 * esp_now_send drops the packet but fires the registered send callback with
 * a success status so EspnowTransmitter's "_espnowSendConfirmed" state
 * machine advances instead of hanging. Receive callbacks are stored but never
 * invoked — no other node can reach this device over ESP-NOW.
 */
#include "espnow.h"
#include <string.h>

static esp_now_recv_cb_t s_recv_cb = 0;
static esp_now_send_cb_t s_send_cb = 0;

int esp_now_init(void)              { return 0; }   /* no radio to configure */
int esp_now_deinit(void)            { s_recv_cb = 0; s_send_cb = 0; return 0; }

int esp_now_register_send_cb(esp_now_send_cb_t cb)  { s_send_cb = cb; return 0; }
int esp_now_unregister_send_cb(void)                { s_send_cb = 0; return 0; }

int esp_now_register_recv_cb(esp_now_recv_cb_t cb)  { s_recv_cb = cb; return 0; }
int esp_now_unregister_recv_cb(void)                { s_recv_cb = 0; return 0; }

int esp_now_send(u8 *da, u8 *data, int len)
{
    (void)da; (void)data; (void)len;
    /* No radio: drop the frame, but tell the transmitter it went out so the
     * mesh send/ACK state machine doesn't stall on a never-confirmed send. */
    if (s_send_cb) s_send_cb(da, 0);
    return 0;
}

int esp_now_add_peer(u8 *mac_addr, u8 role, u8 channel, u8 *key, u8 key_len)
{ (void)mac_addr; (void)role; (void)channel; (void)key; (void)key_len; return 0; }

int esp_now_del_peer(u8 *mac_addr)      { (void)mac_addr; return 0; }

int esp_now_set_self_role(u8 role)      { (void)role; return 0; }
int esp_now_get_self_role(void)         { return ESP_NOW_ROLE_COMBO; }

int esp_now_set_peer_role(u8 *mac_addr, u8 role) { (void)mac_addr; (void)role; return 0; }
int esp_now_get_peer_role(u8 *mac_addr)          { (void)mac_addr; return ESP_NOW_ROLE_COMBO; }

int esp_now_set_peer_channel(u8 *mac_addr, u8 channel) { (void)mac_addr; (void)channel; return 0; }
int esp_now_get_peer_channel(u8 *mac_addr)             { (void)mac_addr; return 1; }

int esp_now_set_peer_key(u8 *mac_addr, u8 *key, u8 key_len)
{ (void)mac_addr; (void)key; (void)key_len; return 0; }

int esp_now_get_peer_key(u8 *mac_addr, u8 *key, u8 *key_len)
{ (void)mac_addr; (void)key; if (key_len) *key_len = 0; return 0; }

u8 *esp_now_fetch_peer(bool restart)    { (void)restart; return 0; }

int esp_now_is_peer_exist(u8 *mac_addr) { (void)mac_addr; return 1; } /* never pruned */

int esp_now_get_cnt_info(u8 *all_cnt, u8 *encrypt_cnt)
{ (void)all_cnt; (void)encrypt_cnt; return 0; }

int esp_now_set_kok(u8 *key, u8 len)    { (void)key; (void)len; return 0; }
