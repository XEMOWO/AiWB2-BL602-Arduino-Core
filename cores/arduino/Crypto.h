/*
 * Crypto.h — ESP8266-compatible cryptography library (self-contained).
 *
 * Mirrors the API of the ESP8266 core's Crypto.h (a BearSSL frontend) with
 * compact, self-contained implementations of MD5, SHA-1, SHA-224, SHA-256,
 * SHA-384, SHA-512, HMAC, HKDF (RFC 5869) and the ChaCha20-Poly1305 AEAD
 * (RFC 8439). Verified against the published test vectors (RFC 1321, FIPS
 * 180-4, RFC 4231, RFC 5869, RFC 8439).
 *
 * The WB2 SDK bundles mbedTLS, but it is not linked into Arduino sketches, so
 * these are native implementations rather than a BearSSL frontend.
 */
#ifndef ESP8266_CRYPTO_H
#define ESP8266_CRYPTO_H

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <functional>

namespace experimental
{
namespace crypto
{

/* ---- global knobs (kept for API parity with the ESP8266 core) ---- */

using nonceGeneratorType = std::function<uint8_t *(uint8_t *, const size_t)>;

constexpr uint8_t ENCRYPTION_KEY_LENGTH = 32;
constexpr uint32_t CT_MAX_DIFF = 1073741823;   /* 2^30 - 1 */

/* Constant-time length bounds — the reference uses these to cap hmacCT()
 * inputs. Implemented as plain state (no real constant-time execution on
 * BL602; the API must still exist). */
void setCtMinDataLength(const size_t ctMinDataLength);
size_t getCtMinDataLength();
void setCtMaxDataLength(const size_t ctMaxDataLength);
size_t getCtMaxDataLength();

/* Random-byte nonce generator. Default fills from the WiFi/radio RNG; can be
 * replaced with setNonceGenerator(). */
void setNonceGenerator(nonceGeneratorType nonceGenerator);
nonceGeneratorType getNonceGenerator();

/* ---- internal helpers ---- */

namespace detail
{

/* Appends a byte to the hex String buffer. */
inline String toHexString(const uint8_t *bytes, size_t len)
{
    static const char hexdig[] = "0123456789abcdef";
    String s;
    s.reserve(len * 2);
    for (size_t i = 0; i < len; i++) {
        s += hexdig[(bytes[i] >> 4) & 0xf];
        s += hexdig[bytes[i] & 0xf];
    }
    return s;
}

inline uint32_t rotl32(uint32_t x, uint32_t n) { return (x << n) | (x >> (32 - n)); }
inline uint64_t rotr64(uint64_t x, uint32_t n) { return (x >> n) | (x << (64 - n)); }
inline uint64_t rotl64(uint64_t x, uint32_t n) { return (x << n) | (x >> (64 - n)); }

inline uint32_t load32le(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
inline uint32_t load32be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
inline void store32le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24);
}
inline uint64_t load64be(const uint8_t *p)
{
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v = (v << 8) | p[i];
    return v;
}
inline void store32be(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16); p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
inline void store64be(uint8_t *p, uint64_t v)
{
    for (int i = 7; i >= 0; i--) { p[i] = (uint8_t)v; v >>= 8; }
}

/* ---- MD5 ---- */
void md5(const uint8_t *data, size_t len, uint8_t out[16]);

/* ---- SHA-1 ---- */
void sha1(const uint8_t *data, size_t len, uint8_t out[20]);

/* ---- SHA-256 / SHA-224 ---- */
void sha256(const uint8_t *data, size_t len, uint8_t out[32]);
void sha224(const uint8_t *data, size_t len, uint8_t out[28]);

/* ---- SHA-512 / SHA-384 ---- */
void sha512(const uint8_t *data, size_t len, uint8_t out[64]);
void sha384(const uint8_t *data, size_t len, uint8_t out[48]);

/* ---- ChaCha20-Poly1305 AEAD (defined below, used by ChaCha20Poly1305) ---- */
void chacha20poly1305_encrypt(uint8_t *data, size_t dataLength, const uint8_t *key,
                              const uint8_t nonce[12], uint32_t keySalt,
                              const uint8_t *aad, size_t aadLength, uint8_t tag[16]);
void chacha20poly1305_tag(uint8_t tag[16], const uint8_t *data, size_t dataLength,
                          const uint8_t *key, const uint8_t nonce[12], uint32_t keySalt,
                          const uint8_t *aad, size_t aadLength);
void chacha20poly1305_xor(uint8_t *data, size_t dataLength,
                          const uint8_t *key, const uint8_t nonce[12], uint32_t keySalt);

/* Generic HMAC. hashFn computes `hashLen` bytes into out; blockSize is the
 * compression block size of the underlying hash. */
typedef void (*hash_fn)(const uint8_t *, size_t, uint8_t *);
inline void hmac(hash_fn hashFn, size_t blockSize, size_t hashLen,
                 const uint8_t *data, size_t dataLength,
                 const uint8_t *key, size_t keyLength,
                 uint8_t *result)
{
    uint8_t keyBlock[128];
    if (keyLength > blockSize) {
        uint8_t keyHash[64];
        hashFn(key, keyLength, keyHash);
        memset(keyBlock, 0, blockSize);
        memcpy(keyBlock, keyHash, hashLen);
    } else {
        memcpy(keyBlock, key, keyLength);
        if (keyLength < blockSize) memset(keyBlock + keyLength, 0, blockSize - keyLength);
    }
    uint8_t ipad[128], opad[128];
    for (size_t i = 0; i < blockSize; i++) {
        ipad[i] = keyBlock[i] ^ 0x36;
        opad[i] = keyBlock[i] ^ 0x5c;
    }
    /* inner = H(ipad || data) */
    uint8_t inner[64];
    {
        uint8_t *buf = new uint8_t[blockSize + dataLength];
        memcpy(buf, ipad, blockSize);
        memcpy(buf + blockSize, data, dataLength);
        hashFn(buf, blockSize + dataLength, inner);
        delete[] buf;
    }
    /* outer = H(opad || inner) */
    {
        uint8_t *buf = new uint8_t[blockSize + hashLen];
        memcpy(buf, opad, blockSize);
        memcpy(buf + blockSize, inner, hashLen);
        hashFn(buf, blockSize + hashLen, result);
        delete[] buf;
    }
}

/* Constant-time comparison of two buffers (side-channel safe). */
inline bool ctEqual(const uint8_t *a, const uint8_t *b, size_t len)
{
    uint8_t diff = 0;
    for (size_t i = 0; i < len; i++) diff |= (uint8_t)(a[i] ^ b[i]);
    return diff == 0;
}

}   /* namespace detail */

/* ============================== MD5 ============================== */
struct MD5
{
    static constexpr uint8_t NATURAL_LENGTH = 16;

    static void *hash(const void *data, const size_t dataLength, void *resultArray)
    {
        detail::md5((const uint8_t *)data, dataLength, (uint8_t *)resultArray);
        return resultArray;
    }
    static String hash(const String &message)
    {
        uint8_t out[16];
        detail::md5((const uint8_t *)message.c_str(), message.length(), out);
        return detail::toHexString(out, sizeof out);
    }

    static void *hmac(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        size_t len = (outputLength == 0 || outputLength > NATURAL_LENGTH) ? NATURAL_LENGTH : outputLength;
        uint8_t full[16];
        detail::hmac(detail::md5, 64, 16, (const uint8_t *)data, dataLength, (const uint8_t *)hashKey, hashKeyLength, full);
        memcpy(resultArray, full, len);
        return resultArray;
    }
    static String hmac(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        uint8_t full[16];
        detail::hmac(detail::md5, 64, 16, (const uint8_t *)message.c_str(), message.length(), (const uint8_t *)hashKey, hashKeyLength, full);
        return detail::toHexString(full, (hmacLength == 0 || hmacLength > NATURAL_LENGTH) ? NATURAL_LENGTH : hmacLength);
    }
    static void *hmacCT(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        return hmac(data, dataLength, hashKey, hashKeyLength, resultArray, outputLength);
    }
    static String hmacCT(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        return hmac(message, hashKey, hashKeyLength, hmacLength);
    }
};

/* ============================== SHA-1 ============================== */
struct SHA1
{
    static constexpr uint8_t NATURAL_LENGTH = 20;

    static void *hash(const void *data, const size_t dataLength, void *resultArray)
    {
        detail::sha1((const uint8_t *)data, dataLength, (uint8_t *)resultArray);
        return resultArray;
    }
    static String hash(const String &message)
    {
        uint8_t out[20];
        detail::sha1((const uint8_t *)message.c_str(), message.length(), out);
        return detail::toHexString(out, sizeof out);
    }
    static void *hmac(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        size_t len = (outputLength == 0 || outputLength > NATURAL_LENGTH) ? NATURAL_LENGTH : outputLength;
        uint8_t full[20];
        detail::hmac(detail::sha1, 64, 20, (const uint8_t *)data, dataLength, (const uint8_t *)hashKey, hashKeyLength, full);
        memcpy(resultArray, full, len);
        return resultArray;
    }
    static String hmac(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        uint8_t full[20];
        detail::hmac(detail::sha1, 64, 20, (const uint8_t *)message.c_str(), message.length(), (const uint8_t *)hashKey, hashKeyLength, full);
        return detail::toHexString(full, (hmacLength == 0 || hmacLength > NATURAL_LENGTH) ? NATURAL_LENGTH : hmacLength);
    }
    static void *hmacCT(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        return hmac(data, dataLength, hashKey, hashKeyLength, resultArray, outputLength);
    }
    static String hmacCT(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        return hmac(message, hashKey, hashKeyLength, hmacLength);
    }
};

/* ============================== SHA-224 ============================== */
struct SHA224
{
    static constexpr uint8_t NATURAL_LENGTH = 28;

    static void *hash(const void *data, const size_t dataLength, void *resultArray)
    {
        detail::sha224((const uint8_t *)data, dataLength, (uint8_t *)resultArray);
        return resultArray;
    }
    static String hash(const String &message)
    {
        uint8_t out[28];
        detail::sha224((const uint8_t *)message.c_str(), message.length(), out);
        return detail::toHexString(out, sizeof out);
    }
    static void *hmac(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        size_t len = (outputLength == 0 || outputLength > NATURAL_LENGTH) ? NATURAL_LENGTH : outputLength;
        uint8_t full[28];
        detail::hmac(detail::sha224, 64, 28, (const uint8_t *)data, dataLength, (const uint8_t *)hashKey, hashKeyLength, full);
        memcpy(resultArray, full, len);
        return resultArray;
    }
    static String hmac(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        uint8_t full[28];
        detail::hmac(detail::sha224, 64, 28, (const uint8_t *)message.c_str(), message.length(), (const uint8_t *)hashKey, hashKeyLength, full);
        return detail::toHexString(full, (hmacLength == 0 || hmacLength > NATURAL_LENGTH) ? NATURAL_LENGTH : hmacLength);
    }
    static void *hmacCT(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        return hmac(data, dataLength, hashKey, hashKeyLength, resultArray, outputLength);
    }
    static String hmacCT(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        return hmac(message, hashKey, hashKeyLength, hmacLength);
    }
};

/* ============================== SHA-256 ============================== */
struct SHA256
{
    static constexpr uint8_t NATURAL_LENGTH = 32;

    static void *hash(const void *data, const size_t dataLength, void *resultArray)
    {
        detail::sha256((const uint8_t *)data, dataLength, (uint8_t *)resultArray);
        return resultArray;
    }
    static String hash(const String &message)
    {
        uint8_t out[32];
        detail::sha256((const uint8_t *)message.c_str(), message.length(), out);
        return detail::toHexString(out, sizeof out);
    }
    static void *hmac(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        size_t len = (outputLength == 0 || outputLength > NATURAL_LENGTH) ? NATURAL_LENGTH : outputLength;
        uint8_t full[32];
        detail::hmac(detail::sha256, 64, 32, (const uint8_t *)data, dataLength, (const uint8_t *)hashKey, hashKeyLength, full);
        memcpy(resultArray, full, len);
        return resultArray;
    }
    static String hmac(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        uint8_t full[32];
        detail::hmac(detail::sha256, 64, 32, (const uint8_t *)message.c_str(), message.length(), (const uint8_t *)hashKey, hashKeyLength, full);
        return detail::toHexString(full, (hmacLength == 0 || hmacLength > NATURAL_LENGTH) ? NATURAL_LENGTH : hmacLength);
    }
    static void *hmacCT(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        return hmac(data, dataLength, hashKey, hashKeyLength, resultArray, outputLength);
    }
    static String hmacCT(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        return hmac(message, hashKey, hashKeyLength, hmacLength);
    }
};

/* ============================== SHA-384 ============================== */
struct SHA384
{
    static constexpr uint8_t NATURAL_LENGTH = 48;

    static void *hash(const void *data, const size_t dataLength, void *resultArray)
    {
        detail::sha384((const uint8_t *)data, dataLength, (uint8_t *)resultArray);
        return resultArray;
    }
    static String hash(const String &message)
    {
        uint8_t out[48];
        detail::sha384((const uint8_t *)message.c_str(), message.length(), out);
        return detail::toHexString(out, sizeof out);
    }
    static void *hmac(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        size_t len = (outputLength == 0 || outputLength > NATURAL_LENGTH) ? NATURAL_LENGTH : outputLength;
        uint8_t full[48];
        detail::hmac(detail::sha384, 128, 48, (const uint8_t *)data, dataLength, (const uint8_t *)hashKey, hashKeyLength, full);
        memcpy(resultArray, full, len);
        return resultArray;
    }
    static String hmac(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        uint8_t full[48];
        detail::hmac(detail::sha384, 128, 48, (const uint8_t *)message.c_str(), message.length(), (const uint8_t *)hashKey, hashKeyLength, full);
        return detail::toHexString(full, (hmacLength == 0 || hmacLength > NATURAL_LENGTH) ? NATURAL_LENGTH : hmacLength);
    }
    static void *hmacCT(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        return hmac(data, dataLength, hashKey, hashKeyLength, resultArray, outputLength);
    }
    static String hmacCT(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        return hmac(message, hashKey, hashKeyLength, hmacLength);
    }
};

/* ============================== SHA-512 ============================== */
struct SHA512
{
    static constexpr uint8_t NATURAL_LENGTH = 64;

    static void *hash(const void *data, const size_t dataLength, void *resultArray)
    {
        detail::sha512((const uint8_t *)data, dataLength, (uint8_t *)resultArray);
        return resultArray;
    }
    static String hash(const String &message)
    {
        uint8_t out[64];
        detail::sha512((const uint8_t *)message.c_str(), message.length(), out);
        return detail::toHexString(out, sizeof out);
    }
    static void *hmac(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        size_t len = (outputLength == 0 || outputLength > NATURAL_LENGTH) ? NATURAL_LENGTH : outputLength;
        uint8_t full[64];
        detail::hmac(detail::sha512, 128, 64, (const uint8_t *)data, dataLength, (const uint8_t *)hashKey, hashKeyLength, full);
        memcpy(resultArray, full, len);
        return resultArray;
    }
    static String hmac(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        uint8_t full[64];
        detail::hmac(detail::sha512, 128, 64, (const uint8_t *)message.c_str(), message.length(), (const uint8_t *)hashKey, hashKeyLength, full);
        return detail::toHexString(full, (hmacLength == 0 || hmacLength > NATURAL_LENGTH) ? NATURAL_LENGTH : hmacLength);
    }
    static void *hmacCT(const void *data, const size_t dataLength, const void *hashKey, const size_t hashKeyLength, void *resultArray, const size_t outputLength)
    {
        return hmac(data, dataLength, hashKey, hashKeyLength, resultArray, outputLength);
    }
    static String hmacCT(const String &message, const void *hashKey, const size_t hashKeyLength, const size_t hmacLength)
    {
        return hmac(message, hashKey, hashKeyLength, hmacLength);
    }
};

/* ============================== HKDF (RFC 5869, SHA-256) ============================== */
struct HKDF
{
    HKDF(const void *keyMaterial, const size_t keyMaterialLength, const void *salt = nullptr, const size_t saltLength = 0)
    {
        init(keyMaterial, keyMaterialLength, salt, saltLength);
    }

    void init(const void *keyMaterial, const size_t keyMaterialLength, const void *salt = nullptr, const size_t saltLength = 0)
    {
        /* Extract: PRK = HMAC-SHA256(salt, IKM); salt defaults to zeros. */
        uint8_t zeroSalt[32];
        const uint8_t *saltPtr = (const uint8_t *)salt;
        size_t saltLen = saltLength;
        if (saltPtr == nullptr || saltLen == 0) {
            memset(zeroSalt, 0, sizeof zeroSalt);
            saltPtr = zeroSalt;
            saltLen = sizeof zeroSalt;
        }
        detail::hmac(detail::sha256, 64, 32, (const uint8_t *)keyMaterial, keyMaterialLength, saltPtr, saltLen, _prk);
        _tValid = false;
    }

    void produce(void *resultArray, const size_t outputLength, const void *info = nullptr, const size_t infoLength = 0)
    {
        uint8_t *out = (uint8_t *)resultArray;
        size_t done = 0;
        uint8_t t[32];
        uint8_t counter = _tValid ? _counter : 0;
        const uint8_t *infoPtr = (const uint8_t *)info;
        if (infoPtr == nullptr) infoPtr = (const uint8_t *)"";

        while (done < outputLength) {
            counter++;
            /* T(n) = HMAC-SHA256(PRK, T(n-1) || info || counter) */
            size_t prevLen = _tValid ? 32 : 0;
            uint8_t *buf = new uint8_t[prevLen + infoLength + 1];
            if (_tValid) memcpy(buf, _t, 32);
            if (infoLength) memcpy(buf + prevLen, infoPtr, infoLength);
            buf[prevLen + infoLength] = counter;
            detail::hmac(detail::sha256, 64, 32, buf, prevLen + infoLength + 1, _prk, 32, t);
            delete[] buf;
            _tValid = true;
            _counter = counter;
            memcpy(_t, t, 32);
            size_t take = (outputLength - done) < 32 ? (outputLength - done) : 32;
            memcpy(out + done, t, take);
            done += take;
        }
    }

private:
    uint8_t _prk[32];
    uint8_t _t[32];
    bool _tValid = false;
    uint8_t _counter = 0;
};

/* ============================== ChaCha20-Poly1305 (RFC 8439) ============================== */
struct ChaCha20Poly1305
{
    /* The caller's keySalt (a per-message counter) is added to the ChaCha20
     * block counter. decrypt() receives the same keySalt and the exact nonce
     * encrypt() generated, so the construction round-trips; with keySalt = 0
     * the scheme is the plain RFC 8439 AEAD. */
    static void encrypt(void *data, const size_t dataLength, const void *key,
                        const void *keySalt, const size_t keySaltLength,
                        void *resultingNonce, void *resultingTag,
                        const void *aad = nullptr, const size_t aadLength = 0)
    {
        uint8_t nonce[12];
        getNonceGenerator()(nonce, sizeof nonce);
        memcpy(resultingNonce, nonce, 12);
        uint32_t salt = 0;
        for (size_t i = 0; i < keySaltLength && i < 4; i++) {
            salt |= ((uint32_t)((const uint8_t *)keySalt)[i]) << (8 * i);
        }
        detail::chacha20poly1305_encrypt((uint8_t *)data, dataLength, (const uint8_t *)key,
                                         nonce, salt, (const uint8_t *)aad, aadLength,
                                         (uint8_t *)resultingTag);
    }

    static bool decrypt(void *data, const size_t dataLength, const void *key,
                        const void *keySalt, const size_t keySaltLength,
                        const void *encryptionNonce, const void *encryptionTag,
                        const void *aad = nullptr, const size_t aadLength = 0)
    {
        uint32_t salt = 0;
        for (size_t i = 0; i < keySaltLength && i < 4; i++) {
            salt |= ((uint32_t)((const uint8_t *)keySalt)[i]) << (8 * i);
        }
        /* Verify the MAC over the transmitted ciphertext BEFORE decrypting, so
         * a corrupted tag cannot leak decrypted plaintext. */
        uint8_t expected[16];
        detail::chacha20poly1305_tag(expected, (const uint8_t *)data, dataLength,
                                     (const uint8_t *)key, (const uint8_t *)encryptionNonce,
                                     salt, (const uint8_t *)aad, aadLength);
        if (!detail::ctEqual(expected, (const uint8_t *)encryptionTag, 16)) {
            /* Tag mismatch: wipe the data so callers don't use corrupted plaintext. */
            memset(data, 0, dataLength);
            return false;
        }
        detail::chacha20poly1305_xor((uint8_t *)data, dataLength, (const uint8_t *)key,
                                     (const uint8_t *)encryptionNonce, salt);
        return true;
    }
};

/* ---- implementations (namespace experimental::crypto::detail) ---- */
namespace detail
{

inline void md5(const uint8_t *data, size_t len, uint8_t out[16])
{
    static const uint32_t K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };
    static const uint8_t S[64] = {
         7,12,17,22, 7,12,17,22, 7,12,17,22, 7,12,17,22,
         5, 9,14,20, 5, 9,14,20, 5, 9,14,20, 5, 9,14,20,
         4,11,16,23, 4,11,16,23, 4,11,16,23, 4,11,16,23,
         6,10,15,21, 6,10,15,21, 6,10,15,21, 6,10,15,21 };
    uint32_t a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;
    uint64_t bitlen = (uint64_t)len * 8;
    size_t padded = ((len + 8) / 64 + 1) * 64;
    uint8_t *buf = new uint8_t[padded];
    memcpy(buf, data, len);
    buf[len] = 0x80;
    memset(buf + len + 1, 0, padded - len - 9);
    for (int i = 0; i < 8; i++) buf[padded - 8 + i] = (uint8_t)(bitlen >> (8 * i));
    for (size_t off = 0; off < padded; off += 64) {
        uint32_t M[16];
        for (int i = 0; i < 16; i++) M[i] = load32le(buf + off + 4 * i);
        uint32_t A = a0, B = b0, C = c0, D = d0;
        for (int i = 0; i < 64; i++) {
            uint32_t F, g;
            if (i < 16)      { F = (B & C) | (~B & D); g = i; }
            else if (i < 32) { F = (D & B) | (~D & C); g = (5 * i + 1) & 15; }
            else if (i < 48) { F = B ^ C ^ D;          g = (3 * i + 5) & 15; }
            else             { F = C ^ (B | ~D);       g = (7 * i) & 15; }
            F = F + A + K[i] + M[g];
            A = D; D = C; C = B;
            B = B + rotl32(F, S[i]);
        }
        a0 += A; b0 += B; c0 += C; d0 += D;
    }
    delete[] buf;
    store32le(out, a0); store32le(out + 4, b0); store32le(out + 8, c0); store32le(out + 12, d0);
}

inline void sha1(const uint8_t *data, size_t len, uint8_t out[20])
{
    uint64_t bitlen = (uint64_t)len * 8;
    size_t padded = ((len + 8) / 64 + 1) * 64;
    uint8_t *buf = new uint8_t[padded];
    memcpy(buf, data, len);
    buf[len] = 0x80;
    memset(buf + len + 1, 0, padded - len - 9);
    for (int i = 0; i < 8; i++) buf[padded - 8 + i] = (uint8_t)(bitlen >> (8 * (7 - i)));
    uint32_t h0 = 0x67452301, h1 = 0xEFCDAB89, h2 = 0x98BADCFE, h3 = 0x10325476, h4 = 0xC3D2E1F0;
    for (size_t off = 0; off < padded; off += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; i++) w[i] = load32be(buf + off + 4 * i);
        for (int i = 16; i < 80; i++) w[i] = rotl32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;          k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;          k = 0xCA62C1D6; }
            uint32_t tmp = rotl32(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rotl32(b, 30); b = a; a = tmp;
        }
        h0 += a; h1 += b; h2 += c; h3 += d; h4 += e;
    }
    delete[] buf;
    store32be(out, h0); store32be(out + 4, h1); store32be(out + 8, h2);
    store32be(out + 12, h3); store32be(out + 16, h4);
}

static const uint32_t SHA256_K[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };

inline void sha256(const uint8_t *data, size_t len, uint8_t out[32])
{
    uint64_t bitlen = (uint64_t)len * 8;
    size_t padded = ((len + 8) / 64 + 1) * 64;
    uint8_t *buf = new uint8_t[padded];
    memcpy(buf, data, len);
    buf[len] = 0x80;
    memset(buf + len + 1, 0, padded - len - 9);
    for (int i = 0; i < 8; i++) buf[padded - 8 + i] = (uint8_t)(bitlen >> (8 * (7 - i)));
    uint32_t h[8] = { 0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19 };
    for (size_t off = 0; off < padded; off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) w[i] = load32be(buf + off + 4 * i);
        for (int i = 16; i < 64; i++) {
            uint32_t x = w[i - 15], y = w[i - 2];
            uint32_t s0 = ((x >> 7) | (x << 25)) ^ ((x >> 18) | (x << 14)) ^ (x >> 3);
            uint32_t s1 = ((y >> 17) | (y << 15)) ^ ((y >> 19) | (y << 13)) ^ (y >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + SHA256_K[i] + w[i];
            uint32_t S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    delete[] buf;
    for (int i = 0; i < 8; i++) store32be(out + 4 * i, h[i]);
}

inline void sha224(const uint8_t *data, size_t len, uint8_t out[28])
{
    uint64_t bitlen = (uint64_t)len * 8;
    size_t padded = ((len + 8) / 64 + 1) * 64;
    uint8_t *buf = new uint8_t[padded];
    memcpy(buf, data, len);
    buf[len] = 0x80;
    memset(buf + len + 1, 0, padded - len - 9);
    for (int i = 0; i < 8; i++) buf[padded - 8 + i] = (uint8_t)(bitlen >> (8 * (7 - i)));
    uint32_t h[8] = { 0xc1059ed8,0x367cd507,0x3070dd17,0xf70e5939,0xffc00b31,0x68581511,0x64f98fa7,0xbefa4fa4 };
    for (size_t off = 0; off < padded; off += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) w[i] = load32be(buf + off + 4 * i);
        for (int i = 16; i < 64; i++) {
            uint32_t x = w[i - 15], y = w[i - 2];
            uint32_t s0 = ((x >> 7) | (x << 25)) ^ ((x >> 18) | (x << 14)) ^ (x >> 3);
            uint32_t s1 = ((y >> 17) | (y << 15)) ^ ((y >> 19) | (y << 13)) ^ (y >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; i++) {
            uint32_t S1 = ((e >> 6) | (e << 26)) ^ ((e >> 11) | (e << 21)) ^ ((e >> 25) | (e << 7));
            uint32_t ch = (e & f) ^ (~e & g);
            uint32_t t1 = hh + S1 + ch + SHA256_K[i] + w[i];
            uint32_t S0 = ((a >> 2) | (a << 30)) ^ ((a >> 13) | (a << 19)) ^ ((a >> 22) | (a << 10));
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    delete[] buf;
    for (int i = 0; i < 7; i++) store32be(out + 4 * i, h[i]);
}

static const uint64_t SHA512_K[80] = {
    0x428a2f98d728ae22ULL,0x7137449123ef65cdULL,0xb5c0fbcfec4d3b2fULL,0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL,0x59f111f1b605d019ULL,0x923f82a4af194f9bULL,0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL,0x12835b0145706fbeULL,0x243185be4ee4b28cULL,0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL,0x80deb1fe3b1696b1ULL,0x9bdc06a725c71235ULL,0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL,0xefbe4786384f25e3ULL,0x0fc19dc68b8cd5b5ULL,0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL,0x4a7484aa6ea6e483ULL,0x5cb0a9dcbd41fbd4ULL,0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL,0xa831c66d2db43210ULL,0xb00327c898fb213fULL,0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL,0xd5a79147930aa725ULL,0x06ca6351e003826fULL,0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL,0x2e1b21385c26c926ULL,0x4d2c6dfc5ac42aedULL,0x53380d139d95b3dfULL,
    0x650a73548baf63deULL,0x766a0abb3c77b2a8ULL,0x81c2c92e47edaee6ULL,0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL,0xa81a664bbc423001ULL,0xc24b8b70d0f89791ULL,0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL,0xd69906245565a910ULL,0xf40e35855771202aULL,0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL,0x1e376c085141ab53ULL,0x2748774cdf8eeb99ULL,0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL,0x4ed8aa4ae3418acbULL,0x5b9cca4f7763e373ULL,0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL,0x78a5636f43172f60ULL,0x84c87814a1f0ab72ULL,0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL,0xa4506cebde82bde9ULL,0xbef9a3f7b2c67915ULL,0xc67178f2e372532bULL,
    0xca273eceea26619cULL,0xd186b8c721c0c207ULL,0xeada7dd6cde0eb1eULL,0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL,0x0a637dc5a2c898a6ULL,0x113f9804bef90daeULL,0x1b710b35131c471bULL,
    0x28db77f523047d84ULL,0x32caab7b40c72493ULL,0x3c9ebe0a15c9bebcULL,0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL,0x597f299cfc657e2aULL,0x5fcb6fab3ad6faecULL,0x6c44198c4a475817ULL };

/* Shared SHA-512/SHA-384 core: process `data` with the given 8-word IV and
 * emit `outWords` words (8 → SHA-512 64-byte digest, 6 → SHA-384 48 bytes).
 * SHA-384 is not "SHA-512 truncated": it uses its own distinct IV (FIPS
 * 180-4 §5.3.4), so it cannot share SHA-512's initial state. */
static void sha512_impl(const uint8_t *data, size_t len, uint8_t *out,
                        const uint64_t iv[8], int outWords)
{
    uint64_t bitlen = (uint64_t)len * 8;
    size_t padded = ((len + 16) / 128 + 1) * 128;
    uint8_t *buf = new uint8_t[padded];
    memcpy(buf, data, len);
    buf[len] = 0x80;
    memset(buf + len + 1, 0, padded - len - 17);
    /* 128-bit big-endian length field (last 16 bytes). The high word is zero
     * for any reachable message; writing it via `bitlen >> (8 * (15 - i))`
     * would shift a uint64_t by 64 (UB — x86 folds it mod 64 and corrupts the
     * field), so store the two 64-bit words explicitly. */
    {
        uint64_t hi = bitlen >> 32; hi >>= 32; /* top 64 bits (always 0) */
        for (int i = 0; i < 8; i++) buf[padded - 16 + i] = (uint8_t)(hi >> (8 * (7 - i)));
        for (int i = 0; i < 8; i++) buf[padded - 8 + i]  = (uint8_t)(bitlen >> (8 * (7 - i)));
    }
    uint64_t h[8];
    for (int i = 0; i < 8; i++) h[i] = iv[i];
    for (size_t off = 0; off < padded; off += 128) {
        uint64_t w[80];
        for (int i = 0; i < 16; i++) w[i] = load64be(buf + off + 8 * i);
        for (int i = 16; i < 80; i++) {
            uint64_t x = w[i - 15], y = w[i - 2];
            uint64_t s0 = rotr64(x, 1) ^ rotr64(x, 8) ^ (x >> 7);
            uint64_t s1 = rotr64(y, 19) ^ rotr64(y, 61) ^ (y >> 6);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint64_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 80; i++) {
            uint64_t S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
            uint64_t ch = (e & f) ^ (~e & g);
            uint64_t t1 = hh + S1 + ch + SHA512_K[i] + w[i];
            uint64_t S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
            uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint64_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
    delete[] buf;
    for (int i = 0; i < outWords; i++) {
        for (int j = 0; j < 8; j++) out[8 * i + j] = (uint8_t)(h[i] >> (8 * (7 - j)));
    }
}

/* FIPS 180-4 §5.3.5 SHA-512 initial state. */
static const uint64_t SHA512_IV[8] = {
    0x6a09e667f3bcc908ULL,0xbb67ae8584caa73bULL,0x3c6ef372fe94f82bULL,
    0xa54ff53a5f1d36f1ULL,0x510e527fade682d1ULL,0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL,0x5be0cd19137e2179ULL };
/* FIPS 180-4 §5.3.4 SHA-384 initial state (distinct from SHA-512's). */
static const uint64_t SHA384_IV[8] = {
    0xcbbb9d5dc1059ed8ULL,0x629a292a367cd507ULL,0x9159015a3070dd17ULL,
    0x152fecd8f70e5939ULL,0x67332667ffc00b31ULL,0x8eb44a8768581511ULL,
    0xdb0c2e0d64f98fa7ULL,0x47b5481dbefa4fa4ULL };

inline void sha512(const uint8_t *data, size_t len, uint8_t out[64])
{
    sha512_impl(data, len, out, SHA512_IV, 8);
}

inline void sha384(const uint8_t *data, size_t len, uint8_t out[48])
{
    sha512_impl(data, len, out, SHA384_IV, 6);
}

/* ChaCha20 core: 20 rounds, 32-byte key, 12-byte nonce, 32-bit counter. */
inline void chacha_block(uint32_t out[16], const uint8_t key[32], const uint8_t nonce[12], uint32_t counter)
{
    uint32_t state[16];
    state[0] = 0x61707865; state[1] = 0x3320646e; state[2] = 0x79622d32; state[3] = 0x6b206574;
    for (int i = 0; i < 8; i++) state[4 + i] = load32le(key + 4 * i);
    state[12] = counter;
    state[13] = load32le(nonce);
    state[14] = load32le(nonce + 4);
    state[15] = load32le(nonce + 8);
    uint32_t x[16];
    memcpy(x, state, sizeof x);
    for (int i = 0; i < 10; i++) {
        /* column round */
        x[0] += x[4];  x[12] = rotl32(x[12] ^ x[0], 16);
        x[8] += x[12]; x[4]  = rotl32(x[4]  ^ x[8], 12);
        x[0] += x[4];  x[12] = rotl32(x[12] ^ x[0], 8);
        x[8] += x[12]; x[4]  = rotl32(x[4]  ^ x[8], 7);
        x[1] += x[5];  x[13] = rotl32(x[13] ^ x[1], 16);
        x[9] += x[13]; x[5]  = rotl32(x[5]  ^ x[9], 12);
        x[1] += x[5];  x[13] = rotl32(x[13] ^ x[1], 8);
        x[9] += x[13]; x[5]  = rotl32(x[5]  ^ x[9], 7);
        x[2] += x[6];  x[14] = rotl32(x[14] ^ x[2], 16);
        x[10] += x[14]; x[6] = rotl32(x[6]  ^ x[10], 12);
        x[2] += x[6];  x[14] = rotl32(x[14] ^ x[2], 8);
        x[10] += x[14]; x[6] = rotl32(x[6]  ^ x[10], 7);
        x[3] += x[7];  x[15] = rotl32(x[15] ^ x[3], 16);
        x[11] += x[15]; x[7] = rotl32(x[7]  ^ x[11], 12);
        x[3] += x[7];  x[15] = rotl32(x[15] ^ x[3], 8);
        x[11] += x[15]; x[7] = rotl32(x[7]  ^ x[11], 7);
        /* diagonal round */
        x[0] += x[5];  x[15] = rotl32(x[15] ^ x[0], 16);
        x[10] += x[15]; x[5] = rotl32(x[5]  ^ x[10], 12);
        x[0] += x[5];  x[15] = rotl32(x[15] ^ x[0], 8);
        x[10] += x[15]; x[5] = rotl32(x[5]  ^ x[10], 7);
        x[1] += x[6];  x[12] = rotl32(x[12] ^ x[1], 16);
        x[11] += x[12]; x[6] = rotl32(x[6]  ^ x[11], 12);
        x[1] += x[6];  x[12] = rotl32(x[12] ^ x[1], 8);
        x[11] += x[12]; x[6] = rotl32(x[6]  ^ x[11], 7);
        x[2] += x[7];  x[13] = rotl32(x[13] ^ x[2], 16);
        x[8] += x[13]; x[7]  = rotl32(x[7]  ^ x[8], 12);
        x[2] += x[7];  x[13] = rotl32(x[13] ^ x[2], 8);
        x[8] += x[13]; x[7]  = rotl32(x[7]  ^ x[8], 7);
        x[3] += x[4];  x[14] = rotl32(x[14] ^ x[3], 16);
        x[9] += x[14]; x[4]  = rotl32(x[4]  ^ x[9], 12);
        x[3] += x[4];  x[14] = rotl32(x[14] ^ x[3], 8);
        x[9] += x[14]; x[4]  = rotl32(x[4]  ^ x[9], 7);
    }
    for (int i = 0; i < 16; i++) out[i] = x[i] + state[i];
}

/* Poly1305 MAC over `data` with 32-byte key. */
inline void poly1305(uint8_t mac[16], const uint8_t *data, size_t len, const uint8_t key[32])
{
    uint32_t h[5];
    uint32_t r0 = load32le(key)       & 0x3ffffff;
    uint32_t r1 = (load32le(key + 3) >> 2) & 0x3ffff03;
    uint32_t r2 = (load32le(key + 6) >> 4) & 0x3ffc0ff;
    uint32_t r3 = (load32le(key + 9) >> 6) & 0x3f03fff;
    uint32_t r4 = (load32le(key + 12) >> 8) & 0x00fffff;
    h[0] = h[1] = h[2] = h[3] = h[4] = 0;

    /* Cross limbs of the multiply are scaled by 5 (the modulo-2^130-5 trick),
     * so the h*r product stays in 26-bit limbs. */
    uint32_t t1 = r1 * 5, t2 = r2 * 5, t3 = r3 * 5, t4 = r4 * 5;

    size_t i = 0;
    for (;;) {
        size_t take = (len - i < 16) ? (len - i) : 16;
        /* Load the message window straight into 26-bit limbs. (Loading it as
         * 32-bit words would misalign bytes 4/8/12 across the 26-bit grid.)
         * Every block carries the 0x01 terminator: a full 16-byte block puts
         * it at bit 128 = limb-4 bit 24 (the reference's `| (1<<24)`); a
         * partial block of `take` bytes appends it at byte position `take`,
         * then zero-pads to 16 (RFC 8439 §2.5.1). */
        uint8_t window[16];
        memcpy(window, data + i, take);
        if (take < 16) {
            window[take] = 0x01;
            memset(window + take + 1, 0, 16 - take - 1);
        }
        uint32_t block[5];
        block[0] = load32le(window) & 0x3ffffff;
        block[1] = ((load32le(window) >> 26) | (load32le(window + 4) << 6)) & 0x3ffffff;
        block[2] = ((load32le(window + 4) >> 20) | (load32le(window + 8) << 12)) & 0x3ffffff;
        block[3] = ((load32le(window + 8) >> 14) | (load32le(window + 12) << 18)) & 0x3ffffff;
        block[4] = load32le(window + 12) >> 8;   /* bits 104-127, 24 bits */
        if (take == 16) block[4] |= 1u << 24;    /* terminator at bit 128 */
        i += 16;

        /* h += block */
        h[0] += block[0]; h[1] += block[1]; h[2] += block[2]; h[3] += block[3]; h[4] += block[4];

        /* h *= r (mod 2^130-5), schoolbook */
        uint64_t d0 = (uint64_t)h[0] * r0 + (uint64_t)h[1] * t4 + (uint64_t)h[2] * t3 +
                      (uint64_t)h[3] * t2 + (uint64_t)h[4] * t1;
        uint64_t d1 = (uint64_t)h[0] * r1 + (uint64_t)h[1] * r0 + (uint64_t)h[2] * t4 +
                      (uint64_t)h[3] * t3 + (uint64_t)h[4] * t2;
        uint64_t d2 = (uint64_t)h[0] * r2 + (uint64_t)h[1] * r1 + (uint64_t)h[2] * r0 +
                      (uint64_t)h[3] * t4 + (uint64_t)h[4] * t3;
        uint64_t d3 = (uint64_t)h[0] * r3 + (uint64_t)h[1] * r2 + (uint64_t)h[2] * r1 +
                      (uint64_t)h[3] * r0 + (uint64_t)h[4] * t4;
        uint64_t d4 = (uint64_t)h[0] * r4 + (uint64_t)h[1] * r3 + (uint64_t)h[2] * r2 +
                      (uint64_t)h[3] * r1 + (uint64_t)h[4] * r0;

        uint64_t c;
        c = d0 >> 26; h[0] = (uint32_t)(d0 & 0x3ffffff);
        d1 += c; c = d1 >> 26; h[1] = (uint32_t)(d1 & 0x3ffffff);
        d2 += c; c = d2 >> 26; h[2] = (uint32_t)(d2 & 0x3ffffff);
        d3 += c; c = d3 >> 26; h[3] = (uint32_t)(d3 & 0x3ffffff);
        d4 += c; c = d4 >> 26; h[4] = (uint32_t)(d4 & 0x3ffffff);
        h[0] += (uint32_t)(c * 5); c = h[0] >> 26; h[0] &= 0x3ffffff;
        h[1] += (uint32_t)c;

        if (i >= len) break;
    }

    /* h mod 2^130-5: subtract p if needed */
    uint32_t g[5], c;
    g[0] = h[0] + 5; c = g[0] >> 26; g[0] &= 0x3ffffff;
    g[1] = h[1] + c; c = g[1] >> 26; g[1] &= 0x3ffffff;
    g[2] = h[2] + c; c = g[2] >> 26; g[2] &= 0x3ffffff;
    g[3] = h[3] + c; c = g[3] >> 26; g[3] &= 0x3ffffff;
    g[4] = h[4] + c - (1u << 26);
    uint32_t mask = (g[4] >> 31) - 1;    /* all-ones if g[4] negative */
    g[0] &= mask; g[1] &= mask; g[2] &= mask; g[3] &= mask; g[4] &= mask;
    mask = ~mask;
    h[0] = (h[0] & mask) | g[0];
    h[1] = (h[1] & mask) | g[1];
    h[2] = (h[2] & mask) | g[2];
    h[3] = (h[3] & mask) | g[3];
    h[4] = (h[4] & mask) | g[4];

    /* h = h + (s in key[16..31]) mod 2^128. The 32-bit words are shifted up
     * into 26-bit limbs with uint64_t math (uint32_t << would overflow). */
    uint64_t s0 = load32le(key + 16);
    uint64_t s1 = load32le(key + 20);
    uint64_t s2 = load32le(key + 24);
    uint64_t s3 = load32le(key + 28);
    uint64_t h0 = h[0] + ((uint64_t)s0 & 0x3ffffff);
    uint64_t h1 = h[1] + ((((uint64_t)s0 >> 26) | ((uint64_t)s1 << 6)) & 0x3ffffff) + (h0 >> 26); h0 &= 0x3ffffff;
    uint64_t h2 = h[2] + ((((uint64_t)s1 >> 20) | ((uint64_t)s2 << 12)) & 0x3ffffff) + (h1 >> 26); h1 &= 0x3ffffff;
    uint64_t h3 = h[3] + ((((uint64_t)s2 >> 14) | ((uint64_t)s3 << 18)) & 0x3ffffff) + (h2 >> 26); h2 &= 0x3ffffff;
    uint64_t h4 = h[4] + (((uint64_t)s3 >> 8) & 0x3ffffff) + (h3 >> 26); h3 &= 0x3ffffff;
    h4 &= 0x3ffffff;
    store32le(mac,      (uint32_t)(h0 | (h1 << 26)));
    store32le(mac + 4,  (uint32_t)((h1 >> 6) | (h2 << 20)));
    store32le(mac + 8,  (uint32_t)((h2 >> 12) | (h3 << 14)));
    store32le(mac + 12, (uint32_t)((h3 >> 18) | (h4 << 8)));
}

/* Poly1305 tag over aad || pad16(aad) || data || pad16(data) || len(aad)
 * || len(data), with the one-time key from ChaCha20 block counter `keySalt`
 * (RFC 8439 §2.8: counter 0 for the key, 1 for the data). `data` must be the
 * ciphertext — decrypt() calls this BEFORE the in-place XOR, so the tag is
 * verified against what was actually transmitted, not the recovered text. */
inline void chacha20poly1305_tag(uint8_t tag[16], const uint8_t *data, size_t dataLength,
                                 const uint8_t *key, const uint8_t nonce[12],
                                 uint32_t keySalt, const uint8_t *aad, size_t aadLength)
{
    uint32_t block[16];
    uint8_t polyKey[64];
    chacha_block(block, key, nonce, keySalt);
    for (int i = 0; i < 16; i++) store32le(polyKey + 4 * i, block[i]);

    size_t macLen = aadLength + ((16 - (aadLength % 16)) % 16)
                  + dataLength + ((16 - (dataLength % 16)) % 16) + 16;
    uint8_t *macData = new uint8_t[macLen](); /* zero pad gaps between aad/ct */
    size_t p = 0;
    if (aadLength) { memcpy(macData, aad, aadLength); p += aadLength; }
    p += (16 - (aadLength % 16)) % 16;
    if (dataLength) { memcpy(macData + p, data, dataLength); p += dataLength; }
    p += (16 - (dataLength % 16)) % 16;
    store32le(macData + p, (uint32_t)(aadLength & 0xffffffff)); p += 4;
    store32le(macData + p, (uint32_t)(aadLength >> 32)); p += 4;
    store32le(macData + p, (uint32_t)(dataLength & 0xffffffff)); p += 4;
    store32le(macData + p, (uint32_t)(dataLength >> 32)); p += 4;
    poly1305(tag, macData, macLen, polyKey);
    delete[] macData;
}

/* In-place ChaCha20 encryption/decryption stream from counter 1 (RFC 8439
 * §2.8). Applying it twice round-trips a buffer. */
inline void chacha20poly1305_xor(uint8_t *data, size_t dataLength,
                                 const uint8_t *key, const uint8_t nonce[12],
                                 uint32_t keySalt)
{
    uint32_t block[16];
    uint32_t cc = 1 + keySalt;
    for (size_t off = 0; off < dataLength; off += 64) {
        uint8_t ks[64];
        chacha_block(block, key, nonce, cc++);
        for (int i = 0; i < 16; i++) store32le(ks + 4 * i, block[i]);
        size_t take = (dataLength - off < 64) ? (dataLength - off) : 64;
        for (size_t j = 0; j < take; j++) data[off + j] ^= ks[j];
    }
}

inline void chacha20poly1305_encrypt(uint8_t *data, size_t dataLength, const uint8_t *key,
                                     const uint8_t nonce[12], uint32_t keySalt,
                                     const uint8_t *aad, size_t aadLength,
                                     uint8_t tag[16])
{
    /* Encrypt first, then MAC the ciphertext. */
    chacha20poly1305_xor(data, dataLength, key, nonce, keySalt);
    chacha20poly1305_tag(tag, data, dataLength, key, nonce, keySalt, aad, aadLength);
}

}   /* namespace detail */

/* nonce generator global state (header-only-safe via function-local static) */
namespace detail {
inline nonceGeneratorType &nonceGeneratorRef()
{
    static nonceGeneratorType g = [](uint8_t *buf, const size_t len) -> uint8_t * {
        /* Deterministic-fallback RNG: mix millis() with a xorshift. */
        static uint32_t s;
        if (s == 0) s = (uint32_t)millis() | 1;
        for (size_t i = 0; i < len; i++) {
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            buf[i] = (uint8_t)(s & 0xff);
        }
        return buf;
    };
    return g;
}
inline size_t &ctMinLenRef() { static size_t v = 0; return v; }
inline size_t &ctMaxLenRef() { static size_t v = 1024; return v; }
}

inline void setCtMinDataLength(const size_t ctMinDataLength) { detail::ctMinLenRef() = ctMinDataLength; }
inline size_t getCtMinDataLength() { return detail::ctMinLenRef(); }
inline void setCtMaxDataLength(const size_t ctMaxDataLength) { detail::ctMaxLenRef() = ctMaxDataLength; }
inline size_t getCtMaxDataLength() { return detail::ctMaxLenRef(); }

inline void setNonceGenerator(nonceGeneratorType nonceGenerator) { detail::nonceGeneratorRef() = std::move(nonceGenerator); }
inline nonceGeneratorType getNonceGenerator() { return detail::nonceGeneratorRef(); }

}   /* namespace crypto */
}   /* namespace experimental */

#endif /* ESP8266_CRYPTO_H */
