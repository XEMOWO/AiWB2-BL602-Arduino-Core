#ifndef DNSServer_h
#define DNSServer_h
#include <WiFiUdp.h>

#ifndef IANA_DNS_PORT
#define IANA_DNS_PORT 53        // AKA domain
constexpr inline uint16_t kIanaDnsPort = IANA_DNS_PORT;
#endif

#define DNS_QR_QUERY 0
#define DNS_QR_RESPONSE 1
#define DNS_OPCODE_QUERY 0

#define DNS_QCLASS_IN 1
#define DNS_QCLASS_ANY 255

#define DNS_QTYPE_A 1
#define DNS_QTYPE_ANY 255

#define MAX_DNSNAME_LENGTH 253
#define MAX_DNS_PACKETSIZE 512

enum class DNSReplyCode
{
  NoError = 0,
  FormError = 1,
  ServerFailure = 2,
  NonExistentDomain = 3,
  NotImplemented = 4,
  Refused = 5,
  YXDomain = 6,
  YXRRSet = 7,
  NXRRSet = 8
};

struct DNSHeader
{
  uint16_t ID;               // identification number
  unsigned char RD : 1;      // recursion desired
  unsigned char TC : 1;      // truncated message
  unsigned char AA : 1;      // authoritative answer
  unsigned char OPCode : 4;  // message_type
  unsigned char QR : 1;      // query/response flag
  unsigned char RCode : 4;   // response code
  unsigned char Z : 3;       // its z! reserved
  unsigned char RA : 1;      // recursion available
  uint16_t QDCount;          // number of question entries
  uint16_t ANCount;          // number of answer entries
  uint16_t NSCount;          // number of authority entries
  uint16_t ARCount;          // number of resource entries
};

class DNSServer
{
  public:
    DNSServer();
    ~DNSServer() {
        stop();
    };
    void processNextRequest();
    void setErrorReplyCode(const DNSReplyCode &replyCode);
    void setTTL(const uint32_t &ttl);
    uint32_t getTTL();
    String getDomainName() { return _domainName; }

    /* DNS forwarder API (ESP8266 DNSServer). The BL602 lwIP has no DNS
     * forwarder, so these manage the state (domain / upstream / on-off) the
     * same way the reference does, but never actually relay lookups upstream:
     * `isForwarding()` reflects whether an upstream DNS was configured, and
     * unmatched queries fall through to the normal captive-portal answer
     * (wildcard or NXDOMAIN) in respondToRequest(). */
    bool enableForwarder(const String &domainName = emptyString, const IPAddress &dns = (uint32_t)0);
    void disableForwarder(const String &domainName = emptyString, bool freeResources = false);
    bool isForwarding() { return _forwarder && _dns.isSet(); }
    void setDNS(const IPAddress& dns) { _dns = dns; }
    IPAddress getDNS() { return _dns; }
    bool isDNSSet() { return _dns.isSet(); }

    // Returns true if successful, false if there are no sockets available
    bool start(const uint16_t &port,
              const String &domainName,
              const IPAddress &resolvedIP,
              const IPAddress &dns = (uint32_t)0);
    // stops the DNS server
    void stop();

  private:
    WiFiUDP _udp;
    uint16_t _port;
    String _domainName;
    IPAddress _dns;
    unsigned char _resolvedIP[4];
    uint32_t _ttl;
    DNSReplyCode _errorReplyCode;
    bool _forwarder = false;

    void downcaseAndRemoveWwwPrefix(String &domainName);
    void replyWithIP(DNSHeader *dnsHeader,
		     unsigned char * query,
		     size_t queryLength);
    void replyWithError(DNSHeader *dnsHeader,
			DNSReplyCode rcode,
			unsigned char *query,
			size_t queryLength);
    void replyWithError(DNSHeader *dnsHeader,
			DNSReplyCode rcode);
    void respondToRequest(uint8_t *buffer, size_t length);
    void writeNBOShort(uint16_t value);
};
#endif
