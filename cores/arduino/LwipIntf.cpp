/*
  LwipIntf.cpp

  Implementation of the LwipIntf helpers (see LwipIntf.h).

  Ported from the ESP8266 core (cores/esp8266/LwipIntf.cpp) to the
  Ai-WB2-12F (BL602). The reference implementation reads/writes the SDK's
  station-hostname global (wifi_station_hostname); here the equivalent is the
  wifi_mgmr hostname, which the SDK copies into each netif->hostname at
  netif_init — so we read it back through the STA netif and set it through
  wifi_mgmr_set_hostname().

  Include order (same rule as WB2WiFiCommon.h): the SDK + lwIP headers come
  before Arduino.h, which LwipIntf.h pulls in via IPAddress.h.
*/

#include <ctype.h>

#include <lwip/ip_addr.h>
#include <lwip/netif.h>
#include <lwip/dhcp.h>
#include <wifi_mgmr_ext.h>

#include "LwipIntf.h"

// args      | esp order    arduino order
// ----      + ---------    -------------
// local_ip  | local_ip     local_ip
// arg1      | gateway      dns1
// arg2      | netmask      gateway
// arg3      | dns1         netmask
//
// result stored into gateway/netmask/dns1

bool LwipIntf::ipAddressReorder(const IPAddress& local_ip, const IPAddress& arg1,
                                const IPAddress& arg2, const IPAddress& arg3, IPAddress& gateway,
                                IPAddress& netmask, IPAddress& dns1)
{
    // To allow compatibility, check first octet of 3rd arg. If 255, interpret as ESP order,
    // otherwise Arduino order.
    gateway = arg1;
    netmask = arg2;
    dns1    = arg3;

    if (netmask[0] != 255)
    {
        // octet is not 255 => interpret as Arduino order
        gateway = arg2;
        netmask = arg3[0] == 0 ? IPAddress(255, 255, 255, 0)
                               : arg3;  // arg order is arduino and 4th arg not given => assign it
                                        // arduino default
        dns1 = arg1;
    }

    // check whether all is IPv4 (or gateway not set)
    if (!(local_ip.isV4() && netmask.isV4() && (!gateway.isSet() || gateway.isV4())))
    {
        return false;
    }

    // ip and gateway must be in the same netmask
    if (gateway.isSet() && (local_ip.v4() & netmask.v4()) != (gateway.v4() & netmask.v4()))
    {
        return false;
    }

    return true;
}

/*
    Get the station DHCP hostname (the wifi_mgmr one, exposed on the STA netif)
    @return hostname
*/
String LwipIntf::hostname(void)
{
    netif* n = wifi_mgmr_sta_netif_get();
    const char* h = (n && n->hostname) ? n->hostname : "espressif";
    return String(h);
}

/*
    Get the station DHCP hostname (the wifi_mgmr one, exposed on the STA netif)
    @return hostname
*/
const char* LwipIntf::getHostname(void)
{
    netif* n = wifi_mgmr_sta_netif_get();
    return (n && n->hostname) ? n->hostname : "espressif";
}

/*
    Set the station DHCP hostname
    @param aHostname max length:32 (SDK limit)
    @return ok
*/
bool LwipIntf::hostname(const char* aHostname)
{
    /*
        RFC952 (see the ESP8266 reference implementation):
        24 chars max, a..z A..Z 0..9 '-' only, no '-' as last char.
    */

    size_t len = strlen(aHostname);

    if (len == 0 || len > 32)
    {
        // sdk limit is 32
        return false;
    }

    // check RFC compliance
    bool compliant = (len <= 24);
    for (size_t i = 0; compliant && i < len; i++)
        if (!isalnum(aHostname[i]) && aHostname[i] != '-')
        {
            compliant = false;
        }
    if (aHostname[len - 1] == '-')
    {
        compliant = false;
    }

    if (wifi_mgmr_set_hostname(const_cast<char*>(aHostname)) != 0)
    {
        return false;
    }

    bool ret = true;

    // now we should inform dhcp server for this change, using lwip_renew()
    // looping through all existing interfaces
    for (netif* intf = netif_list; intf; intf = intf->next)
    {
        // unconditionally update all known interfaces
        intf->hostname = aHostname;

        if (netif_dhcp_data(intf) != nullptr)
        {
            // renew already started DHCP leases
            err_t lwipret = dhcp_renew(intf);
            if (lwipret != ERR_OK)
            {
                ret = false;
            }
        }
    }

    return ret && compliant;
}

/*
    Non-OS-SDK wifi_get_ip_info(): snapshot the STA/AP interface addresses
    straight from its lwIP netif (the same source wifi_mgmr fills). Used by
    LEAmDNS to accept only mDNS queries originating on a local subnet.
*/
extern "C" bool wifi_get_ip_info(uint8_t if_index, struct ip_info* info)
{
    struct netif* n = (if_index == STATION_IF) ? wifi_mgmr_sta_netif_get()
                                               : wifi_mgmr_ap_netif_get();
    if (!n || !info)
    {
        return false;
    }
    info->ip.addr      = ip_2_ip4(&n->ip_addr)->addr;
    info->netmask.addr = ip_2_ip4(&n->netmask)->addr;
    info->gw.addr      = ip_2_ip4(&n->gw)->addr;
    return true;
}
