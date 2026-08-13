/*
 * IRremote.cpp — NEC carrier gating and edge-time decoding.
 *
 * IRsend gates the hardware PWM (33 % duty at 38 kHz) with absolute micros()
 * deadlines, so pulse lengths are accurate even if an ISR runs mid-pulse.
 *
 * IRrecv's ISR records the duration between successive edges on the TSOP
 * output. Durations longer than 50 ms are treated as inter-frame idle and
 * dropped (the last edge timestamp is still kept, so the next recorded
 * duration is the 9 ms header mark). decode() matches the NEC header, then
 * reads 32 bits: a > 1100 us space marks a 1 bit, a short space a 0.
 *
 * bl_pwm.h has no extern "C" guard — wrap it here.
 */
#include "IRremote.h"

extern "C" {
#include <bl_pwm.h>
}

#define NEC_CARRIER_HZ  38000
#define NEC_CARRIER_DUTY 33.0f /* percent: strong IR output */

#define NEC_HDR_MARK   9000
#define NEC_HDR_SPACE  4500
#define NEC_BIT_MARK   562
#define NEC_BIT_ONE    1687
#define NEC_BIT_ZERO   562

/* ---- receive ring (shared with the ISR) ---- */

static volatile uint32_t s_last_us = 0;
static volatile uint16_t s_dur[IR_RECV_BUFSIZE];
static volatile uint8_t  s_head = 0;
static volatile uint8_t  s_tail = 0;

static inline uint8_t ir_next(uint8_t i)
{
    return (uint8_t)((i + 1) & (IR_RECV_BUFSIZE - 1));
}

/* ---------------- IRsend ---------------- */

void IRsend::_carrier(bool on)
{
    bl_pwm_set_duty(_ch, on ? NEC_CARRIER_DUTY : 0.0f);
}

void IRsend::begin(void)
{
    /* Re-init if the frequency changed since the last call. */
    bl_pwm_init(_ch, _pin, (uint32_t)_freq);
    _carrier(false);
}

void IRsend::_mark(uint32_t us)
{
    uint32_t t = micros();
    _carrier(true);
    while ((int32_t)(micros() - t) < (int32_t)us) {
        /* busy wait */
    }
    _carrier(false);
}

void IRsend::_space(uint32_t us)
{
    uint32_t t = micros();
    while ((int32_t)(micros() - t) < (int32_t)us) {
        /* busy wait */
    }
}

void IRsend::sendNEC(uint32_t data, uint8_t nbits)
{
    uint8_t i;

    _mark(NEC_HDR_MARK);
    _space(NEC_HDR_SPACE);
    for (i = 0; i < nbits; i++) {
        _mark(NEC_BIT_MARK);
        if (data & 0x01) {
            _space(NEC_BIT_ONE);
        } else {
            _space(NEC_BIT_ZERO);
        }
        data >>= 1;
    }
    _mark(NEC_BIT_MARK); /* trailing stop pulse */
}

void IRsend::sendRaw(const uint16_t *buf, uint16_t len, uint16_t hz)
{
    uint16_t i;

    if (_freq != hz) {
        _freq = hz;
        bl_pwm_init(_ch, _pin, hz);
    }
    for (i = 0; i < len; i++) {
        if (i & 1) {
            _space(buf[i]);
        } else {
            _mark(buf[i]);
        }
    }
}

/* ---------------- IRrecv ---------------- */

IRrecv *IRrecv::s_self = NULL;

void IRrecv::isr(void)
{
    uint32_t now = micros();
    uint32_t d = now - s_last_us;
    uint8_t nxt;

    s_last_us = now;
    if (d > IR_IDLE_MAX_US) {
        return; /* inter-frame idle: record the edge, drop the duration */
    }
    if (d > 0xFFFF) {
        d = 0xFFFF;
    }
    nxt = ir_next(s_head);
    if (nxt == s_tail) {
        s_tail = ir_next(s_tail); /* ring full: drop the oldest */
    }
    s_dur[s_head] = (uint16_t)d;
    s_head = nxt;
}

void IRrecv::enableIRIn(void)
{
    s_self = this;
    s_head = s_tail = 0;
    s_last_us = micros();
    attachInterrupt(_pin, IRrecv::isr, CHANGE);
}

void IRrecv::disableIRIn(void)
{
    detachInterrupt(_pin);
    s_self = NULL;
}

void IRrecv::resume(void)
{
    s_tail = s_head; /* drop everything buffered so far */
}

uint8_t IRrecv::available(void) const
{
    return (uint8_t)((s_head - s_tail) & (IR_RECV_BUFSIZE - 1));
}

bool IRrecv::decode(IRResults *results)
{
    uint8_t tail, head, avail;
    uint32_t d0, d1;

    /* snapshot the ring state */
    noInterrupts();
    head = s_head;
    tail = s_tail;
    interrupts();
    avail = (uint8_t)((head - tail) & (IR_RECV_BUFSIZE - 1));

    if (avail < 3) {
        return false;
    }

    d0 = s_dur[tail];
    d1 = s_dur[ir_next(tail)];

    if (d0 < 4000 || d0 > 14000 || d1 < 1400 || d1 > 5200) {
        /* not an NEC header; drop one duration and let the next try */
        s_tail = ir_next(tail);
        return false;
    }

    if (d1 > 3000) { /* 4.5 ms space -> full data frame */
        uint32_t data = 0;
        uint8_t i;

        if (avail < 66) { /* header (2) + 32 bits (64) */
            return false;
        }
        for (i = 0; i < 32; i++) {
            uint32_t m = s_dur[(uint8_t)(tail + 2 + 2 * i)];
            uint32_t sp = s_dur[(uint8_t)(tail + 3 + 2 * i)];
            if (m < 200 || m > 1000 || sp < 300 || sp > 2500) {
                /* not a valid NEC bit; drop the header and re-sync */
                s_tail = ir_next(tail);
                return false;
            }
            if (sp > 1100) {
                data |= (1UL << i); /* long space = 1 */
            }
        }
        if (results) {
            results->value = data;
            results->repeat = false;
        }
        s_tail = (uint8_t)(tail + 66);
        return true;
    }

    /* ~2.25 ms space -> NEC repeat frame: header + one trailing mark */
    if (avail >= 3) {
        if (results) {
            results->value = 0;
            results->repeat = true;
        }
        s_tail = (uint8_t)(tail + 3);
        return true;
    }
    return false;
}
