/*
 * Self-contained SHA-1 implementation exposing the BearSSL procedural API
 * (br_sha1_context / br_sha1_init / br_sha1_update / br_sha1_out / ...) that
 * the ESP8266 core's Hash library expects from `<bearssl/bearssl_hash.h>`.
 *
 * The Ai-WB2-12F (BL602) core does not ship the ESP8266 SDK's prebuilt
 * libbearssl.a, so the few symbols the Hash library needs are provided here,
 * inside the library, to keep the port self-contained. The implementation is
 * derived from BearSSL's sha1.c (Copyright (c) 2016 Thomas Pornin
 * <pornin@bolet.org>, MIT-style license) with the internal helper macros that
 * normally live in BearSSL's inner.h inlined, and PROGMEM dropped (a no-op on
 * this XIP target). The context layout matches bearssl_hash.h exactly.
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bearssl_hash.h"

/* ---- local big-endian helpers (from BearSSL inner.h) ---- */

static void
range_dec32be(uint32_t *dst, size_t num, const void *src)
{
	const unsigned char *buf = src;

	while (num -- > 0) {
		*dst = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
			| ((uint32_t)buf[2] << 8) | (uint32_t)buf[3];
		dst ++;
		buf += 4;
	}
}

static void
enc64be(void *dst, uint64_t x)
{
	unsigned char *buf = dst;

	buf[0] = (unsigned char)(x >> 56);
	buf[1] = (unsigned char)(x >> 48);
	buf[2] = (unsigned char)(x >> 40);
	buf[3] = (unsigned char)(x >> 32);
	buf[4] = (unsigned char)(x >> 24);
	buf[5] = (unsigned char)(x >> 16);
	buf[6] = (unsigned char)(x >> 8);
	buf[7] = (unsigned char)x;
}

static void
range_enc32be(void *dst, const uint32_t *src, size_t num)
{
	unsigned char *buf = dst;

	while (num -- > 0) {
		uint32_t x = *src ++;

		buf[0] = (unsigned char)(x >> 24);
		buf[1] = (unsigned char)(x >> 16);
		buf[2] = (unsigned char)(x >> 8);
		buf[3] = (unsigned char)x;
		buf += 4;
	}
}

/* ---- SHA-1 core ---- */

#define F(B, C, D)     ((((C) ^ (D)) & (B)) ^ (D))
#define G(B, C, D)     ((B) ^ (C) ^ (D))
#define H(B, C, D)     (((D) & (C)) | (((D) | (C)) & (B)))
#define I(B, C, D)     G(B, C, D)

#define ROTL(x, n)    (((x) << (n)) | ((x) >> (32 - (n))))

#define K1     ((uint32_t)0x5A827999)
#define K2     ((uint32_t)0x6ED9EBA1)
#define K3     ((uint32_t)0x8F1BBCDC)
#define K4     ((uint32_t)0xCA62C1D6)

static const uint32_t sha1_IV[5] = {
	0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0
};

static void
sha1_round(const unsigned char *buf, uint32_t *val)
{
	uint32_t m[80];
	uint32_t a, b, c, d, e;
	int i;

	a = val[0];
	b = val[1];
	c = val[2];
	d = val[3];
	e = val[4];
	range_dec32be(m, 16, buf);
	for (i = 16; i < 80; i ++) {
		uint32_t x = m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16];
		m[i] = ROTL(x, 1);
	}

	for (i = 0; i < 20; i += 5) {
		e += ROTL(a, 5) + F(b, c, d) + K1 + m[i + 0]; b = ROTL(b, 30);
		d += ROTL(e, 5) + F(a, b, c) + K1 + m[i + 1]; a = ROTL(a, 30);
		c += ROTL(d, 5) + F(e, a, b) + K1 + m[i + 2]; e = ROTL(e, 30);
		b += ROTL(c, 5) + F(d, e, a) + K1 + m[i + 3]; d = ROTL(d, 30);
		a += ROTL(b, 5) + F(c, d, e) + K1 + m[i + 4]; c = ROTL(c, 30);
	}
	for (i = 20; i < 40; i += 5) {
		e += ROTL(a, 5) + G(b, c, d) + K2 + m[i + 0]; b = ROTL(b, 30);
		d += ROTL(e, 5) + G(a, b, c) + K2 + m[i + 1]; a = ROTL(a, 30);
		c += ROTL(d, 5) + G(e, a, b) + K2 + m[i + 2]; e = ROTL(e, 30);
		b += ROTL(c, 5) + G(d, e, a) + K2 + m[i + 3]; d = ROTL(d, 30);
		a += ROTL(b, 5) + G(c, d, e) + K2 + m[i + 4]; c = ROTL(c, 30);
	}
	for (i = 40; i < 60; i += 5) {
		e += ROTL(a, 5) + H(b, c, d) + K3 + m[i + 0]; b = ROTL(b, 30);
		d += ROTL(e, 5) + H(a, b, c) + K3 + m[i + 1]; a = ROTL(a, 30);
		c += ROTL(d, 5) + H(e, a, b) + K3 + m[i + 2]; e = ROTL(e, 30);
		b += ROTL(c, 5) + H(d, e, a) + K3 + m[i + 3]; d = ROTL(d, 30);
		a += ROTL(b, 5) + H(c, d, e) + K3 + m[i + 4]; c = ROTL(c, 30);
	}
	for (i = 60; i < 80; i += 5) {
		e += ROTL(a, 5) + I(b, c, d) + K4 + m[i + 0]; b = ROTL(b, 30);
		d += ROTL(e, 5) + I(a, b, c) + K4 + m[i + 1]; a = ROTL(a, 30);
		c += ROTL(d, 5) + I(e, a, b) + K4 + m[i + 2]; e = ROTL(e, 30);
		b += ROTL(c, 5) + I(d, e, a) + K4 + m[i + 3]; d = ROTL(d, 30);
		a += ROTL(b, 5) + I(c, d, e) + K4 + m[i + 4]; c = ROTL(c, 30);
	}

	val[0] += a;
	val[1] += b;
	val[2] += c;
	val[3] += d;
	val[4] += e;
}

/* ---- BearSSL procedural API for SHA-1 ---- */

void
br_sha1_init(br_sha1_context *cc)
{
	cc->vtable = &br_sha1_vtable;
	memcpy(cc->val, sha1_IV, sizeof cc->val);
	cc->count = 0;
}

void
br_sha1_update(br_sha1_context *cc, const void *data, size_t len)
{
	const unsigned char *buf;
	size_t ptr;

	buf = data;
	ptr = (size_t)cc->count & 63;
	while (len > 0) {
		size_t clen;

		clen = 64 - ptr;
		if (clen > len) {
			clen = len;
		}
		memcpy(cc->buf + ptr, buf, clen);
		ptr += clen;
		buf += clen;
		len -= clen;
		cc->count += (uint64_t)clen;
		if (ptr == 64) {
			sha1_round(cc->buf, cc->val);
			ptr = 0;
		}
	}
}

void
br_sha1_out(const br_sha1_context *cc, void *dst)
{
	unsigned char buf[64];
	uint32_t val[5];
	size_t ptr;

	ptr = (size_t)cc->count & 63;
	memcpy(buf, cc->buf, ptr);
	memcpy(val, cc->val, sizeof val);
	buf[ptr ++] = 0x80;
	if (ptr > 56) {
		memset(buf + ptr, 0, 64 - ptr);
		sha1_round(buf, val);
		memset(buf, 0, 56);
	} else {
		memset(buf + ptr, 0, 56 - ptr);
	}
	enc64be(buf + 56, cc->count << 3);
	sha1_round(buf, val);
	range_enc32be(dst, val, 5);
}

uint64_t
br_sha1_state(const br_sha1_context *cc, void *dst)
{
	range_enc32be(dst, cc->val, 5);
	return cc->count;
}

void
br_sha1_set_state(br_sha1_context *cc, const void *stb, uint64_t count)
{
	range_dec32be(cc->val, 5, stb);
	cc->count = count;
}

const br_hash_class br_sha1_vtable = {
	sizeof(br_sha1_context),
	BR_HASHDESC_ID(br_sha1_ID)
		| BR_HASHDESC_OUT(20)
		| BR_HASHDESC_STATE(20)
		| BR_HASHDESC_LBLEN(6)
		| BR_HASHDESC_MD_PADDING
		| BR_HASHDESC_MD_PADDING_BE,
	(void (*)(const br_hash_class **))&br_sha1_init,
	(void (*)(const br_hash_class **, const void *, size_t))&br_sha1_update,
	(void (*)(const br_hash_class *const *, void *))&br_sha1_out,
	(uint64_t (*)(const br_hash_class *const *, void *))&br_sha1_state,
	(void (*)(const br_hash_class **, const void *, uint64_t))
		&br_sha1_set_state
};
