/*
  LwipDhcpServer.h - DhcpServer control class (ESP8266-compatible).
  Copyright (c) 2018 Espressif, the ESP8266 core.

  The ESP8266's LwipDhcpServer owns the lwIP DHCP server pbuf state. On BL602
  the DHCP server runs inside the SDK's wifi_mgmr AP path
  (wifi_mgmr_ap_start(..., use_dhcp=1)), so this is a control-plane shim:
  begin()/end() flip the SDK AP DHCP on/off and the status accessors report
  what the SDK reports. The SDK's DHCP server only exists while a softAP is
  active and has no hook to append vendor/custom options, so
  onSendOptions()/OptionsBuffer/offered-lease state are stored but never drive
  the wire traffic - custom-offer sketches still compile and run, they just get
  the SDK's default DHCP options.

  The API mirrors cores/esp8266/LwipDhcpServer.h from the ESP8266 core so
  third-party code (RangeExtender, CustomOffer, StaticLease) compiles unchanged.
 */
#ifndef LwipDhcpServer_h
#define LwipDhcpServer_h

#include <IPAddress.h>
#include <stdint.h>
#include <initializer_list>
#include <functional>

#include <lwip/netif.h>   /* ip4_addr_t/ip_addr_t for setDns() & OptionsBuffer::add();
                             also brings the netif_ip4_addr() macros the
                             CustomOffer example calls on getNetif()'s result */

class DhcpServer {
public:
    static constexpr int    DefaultLeaseTime = 720;         /* minutes */
    static constexpr uint32 MagicCookie      = 0x63538263;  /* RFC1497 magic cookie */

    struct OptionsBuffer {
        OptionsBuffer(uint8_t* begin, uint8_t* end) : _it(begin), _begin(begin), _end(end) { }

        OptionsBuffer& add(uint8_t code, const uint8_t* data, size_t size)
        {
            if (_it != _end) { *_it++ = code; }
            for (size_t i = 0; i < size && _it != _end; i++) { *_it++ = data[i]; }
            return *this;
        }

        OptionsBuffer& add(uint8_t code, const char* data, size_t size)
        {
            return add(code, reinterpret_cast<const uint8_t*>(data), size);
        }

        template<size_t Size>
        OptionsBuffer& add(uint8_t code, const char (&data)[Size])
        {
            return add(code, &data[0], Size - 1);
        }

        template<size_t Size>
        OptionsBuffer& add(uint8_t code, const uint8_t (&data)[Size])
        {
            return add(code, &data[0], Size);
        }

        OptionsBuffer& add(uint8_t code, std::initializer_list<uint8_t> data)
        {
            return add(code, data.begin(), data.size());
        }

        OptionsBuffer& add(uint8_t code, const ip4_addr_t* addr)
        {
            return add(code,
                       { ip4_addr1(addr), ip4_addr2(addr), ip4_addr3(addr), ip4_addr4(addr) });
        }

        OptionsBuffer& add(uint8_t code, uint8_t value)
        {
            return add(code, { value });
        }

        OptionsBuffer& add(uint8_t code, uint16_t value)
        {
            return add(code, { static_cast<uint8_t>((value >> 8) & 0xff),
                               static_cast<uint8_t>(value & 0xff) });
        }

        OptionsBuffer& add(uint8_t code, uint32_t value)
        {
            return add(code, { static_cast<uint8_t>((value >> 24) & 0xff),
                               static_cast<uint8_t>((value >> 16) & 0xff),
                               static_cast<uint8_t>((value >> 8) & 0xff),
                               static_cast<uint8_t>(value & 0xff) });
        }

        OptionsBuffer& add(uint8_t code)
        {
            if (_it != _end) { *_it++ = code; }
            return *this;
        }

    private:
        uint8_t* _it;
        uint8_t* _begin;
        uint8_t* _end;
    };

    /* Reference type is a plain function pointer, but CustomOffer passes a
     * generic lambda (`[](const DhcpServer&, auto& options)`), which cannot
     * convert to a function pointer - so onSendOptions() below is templated and
     * stores into a std::function instead. Keep the typedef for API compat. */
    using OptionsBufferHandler = void (*)(const DhcpServer&, OptionsBuffer&);

    DhcpServer() : _started(false), _leaseTime(DefaultLeaseTime), _offerRouter(true), _dns() {}

    /* On BL602 the SDK AP DHCP only exists while a softAP is up; these toggle
     * the SDK DHCP flags and only take effect with an active AP. */
    bool begin(IPAddress ip, IPAddress netmask);
    void end();
    void tick();
    void reset();
    bool isRunning() const { return _started; }

    /* ESP8266 status(): 0 while started, 1 when not. */
    int status() const { return _started ? 0 : 1; }
    bool isStarted() const { return _started; }

    /* A stable netif whose ip_addr tracks the current softAP IP (so
     * `netif_ip4_addr(getNetif())` yields the AP's gateway address). */
    netif* getNetif() const;

    void setRouter(bool value) { _offerRouter = value; }
    bool getRouter() const { return _offerRouter; }

    void setDns(ip4_addr_t addr) { _dns = addr; }
    ip4_addr_t getDns() const { return _dns; }

    void resetLeaseTime() { _leaseTime = DefaultLeaseTime; }
    void setLeaseTime(uint32_t minutes) { _leaseTime = minutes; }
    uint32_t getLeaseTime() const { return _leaseTime; }

    /* Store the custom-offer callback. The SDK's internal DHCP server cannot
     * append vendor options, so it is never invoked - compile-compatible only. */
    template<typename Handler>
    void onSendOptions(Handler handler)
    {
        _optionsHandler = std::move(handler);
    }

    uint8_t* getPoolStart()       { return _poolStart; }
    uint8_t* getPoolEnd()         { return _poolEnd; }
    uint8_t* getLease()           { return _lease; }
    uint8_t* getLeaseEnd()        { return _leaseEnd; }
    IPAddress getNetworkMask() const { return _netmask; }

    /* Static lease (StaticLease.ino): pin `macaddr` to the next pool address.
     * The SDK's AP DHCP server has no per-MAC static-lease table, so this is a
     * compile-compatible no-op that accepts the lease. */
    bool add_dhcps_lease(uint8_t* macaddr);
    bool remove_dhcps_lease(uint8_t* macaddr);

private:
    bool _started;
    uint32_t _leaseTime;
    bool _offerRouter;
    ip4_addr_t _dns;                      /* offered DNS, never sent */
    uint8_t _poolStart[4];
    uint8_t _poolEnd[4];
    uint8_t _lease[4];
    uint8_t _leaseEnd[4];
    IPAddress _netmask;
    std::function<void(const DhcpServer&, OptionsBuffer&)> _optionsHandler;
};

/* The one global DHCP-server object, matching the non-OS SDK's `dhcpSoftAP`
 * (older StaticLease sketches call dhcpSoftAP.add_dhcps_lease() directly).
 * WiFi.softAPDhcpServer() returns this same instance. */
extern DhcpServer dhcpSoftAP;

#endif /* LwipDhcpServer_h */
