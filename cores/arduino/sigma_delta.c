/*
 * sigma_delta.c — BL602 no-op backend for the ESP8266 sigma-delta API.
 *
 * The BL602 has no sigma-delta modulator peripheral exposed to GPIO. The
 * functions exist (declared in sigma_delta.h) so ESP8266 sketches like
 * SigmaDeltaDemo compile and run without crashing; they report the
 * "not in use" state (freq 0 / duty 0 / nothing attached) rather than
 * emitting a signal.
 */
#include "sigma_delta.h"

void sigmaDeltaEnable(void)   { /* no-op */ }
void sigmaDeltaDisable(void)  { /* no-op */ }

uint32_t sigmaDeltaSetup(uint8_t channel, uint32_t freq)
{
    (void)channel; (void)freq;
    return 0; /* unsupported: report 0 so callers see "not active" */
}

void sigmaDeltaWrite(uint8_t channel, uint8_t duty)
{
    (void)channel; (void)duty; /* no-op */
}

uint8_t sigmaDeltaRead(uint8_t channel)
{
    (void)channel;
    return 0;
}

void sigmaDeltaAttachPin(uint8_t pin, uint8_t channel)
{
    (void)pin; (void)channel; /* no-op */
}

void sigmaDeltaDetachPin(uint8_t pin)
{
    (void)pin; /* no-op */
}

bool sigmaDeltaIsPinAttached(uint8_t pin)
{
    (void)pin;
    return false;
}

uint8_t sigmaDeltaGetPrescaler(void)
{
    return 0;
}

void sigmaDeltaSetPrescaler(uint8_t prescaler)
{
    (void)prescaler; /* no-op */
}
