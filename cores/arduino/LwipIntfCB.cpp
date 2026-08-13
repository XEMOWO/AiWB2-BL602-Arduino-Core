/*
  LwipIntfCB.cpp

  LwipIntf::statusChangeCB/stateUpCB/stateDownCB implementation.

  Ported from the ESP8266 core (cores/esp8266/LwipIntfCB.cpp) to the
  Ai-WB2-12F (BL602). On the reference the wifi glue calls a weak
  `netif_status_changed()` whenever a netif status callback fires; here the
  SDK wifi driver installs its own netif->status_callback (a single slot) in
  bl606a0_wifi_netif_init(), so we chain our dispatcher BEHIND it:

      netif->status_callback  =  our dispatcher
                                  |-> SDK's saved callback  (IP event plumbing)
                                  +-> registered mDNS-style callbacks

  Each wifi netif is wired once, on the first statusChangeCB() registration.
  ESP8266mDNS calls it from begin(), i.e. after WiFi has come up, so the
  STA/AP netifs already exist.

  Include order (same rule as WB2WiFiCommon.h): the SDK + lwIP headers come
  before Arduino.h, which LwipIntf.h pulls in via IPAddress.h.
*/

#include <lwip/netif.h>
#include <wifi_mgmr_ext.h>

#include "LwipIntf.h"

/* Callback slots, like the reference core. */
static constexpr size_t LwipIntfCallbacks = 3;

static LwipIntf::CBType callbacks[LwipIntfCallbacks];
static size_t size = 0;

/* Saved SDK status callbacks, keyed per netif (at most the two wifi ones). */
static constexpr size_t MaxWiredNetifs = 4;

struct WiredNetif {
    netif* n;
    netif_status_callback_fn prev;
};
static WiredNetif wired[MaxWiredNetifs];
static unsigned wired_count = 0;

/* Called (as netif->status_callback) whenever a wifi interface goes up/down
 * or its address changes: run the SDK's own callback first, then the
 * LwipIntf clients (e.g. mDNS restarting its responder). */
extern "C" void netif_status_changed(struct netif* netif)
{
    for (unsigned i = 0; i < wired_count; ++i)
        if (wired[i].n == netif && wired[i].prev)
        {
            wired[i].prev(netif);
        }

    for (size_t index = 0; index < size; ++index)
    {
        callbacks[index](netif);
    }
}

/* Chain our dispatcher behind whatever status_callback the SDK installed. */
static void wire_netif(struct netif* n)
{
    if (!n)
    {
        return;
    }
    for (unsigned i = 0; i < wired_count; ++i)
    {
        if (wired[i].n == n)
        {
            return;  /* already wired */
        }
    }
    if (wired_count >= MaxWiredNetifs)
    {
        return;
    }
    wired[wired_count].n    = n;
    wired[wired_count].prev = n->status_callback;
    n->status_callback      = netif_status_changed;
    ++wired_count;
}

bool LwipIntf::statusChangeCB(LwipIntf::CBType cb)
{
    if (size < LwipIntfCallbacks)
    {
        callbacks[size++] = std::move(cb);

        // make sure the wifi interfaces fire netif_status_changed on changes
        wire_netif(wifi_mgmr_sta_netif_get());
        wire_netif(wifi_mgmr_ap_netif_get());
        return true;
    }

    return false;
}

bool LwipIntf::stateUpCB(LwipIntf::CBType cb)
{
    return statusChangeCB(
        [cb](netif* interface)
        {
            if (netif_is_up(interface))
            {
                cb(interface);
            }
        });
}

bool LwipIntf::stateDownCB(LwipIntf::CBType cb)
{
    return statusChangeCB(
        [cb](netif* interface)
        {
            if (!netif_is_up(interface))
            {
                cb(interface);
            }
        });
}
