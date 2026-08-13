/*
 * Esp.cpp — ESP class implementation (see Esp.h).
 *
 * All core C helpers used here are declared in Arduino.h (extern "C").
 */
#include "Esp.h"
#include "spi_flash.h" /* SPI_FLASH_SEC_SIZE, bl_flash_* */

/* ESP8266 non-OS-SDK global that DNSServer and a few libs read directly.
 * BL602 reports power-on; DNSServer only starts when reason is deep-sleep
 * wake (REASON_DEEP_SLEEP_AWAKE <= reason), matching "run after deep sleep"
 * semantics. */
struct rst_info resetInfo = { .reason = REASON_DEFAULT_RST };

uint32_t EspClass::getFreeHeap(void)      { return esp_get_free_heap_size(); }
uint32_t EspClass::getVcc(void)           { return 3300; /* mV, nominal 3V3 */ }
uint32_t EspClass::getFreeContStack(void) { return 0; }
void EspClass::resetFreeContStack(void)   { /* no continuation-stack watermark on BL602 */ }
uint32_t EspClass::getChipId(void)        { return esp_get_chip_id(); }

uint32_t EspClass::random(void)
{
    return RANDOM_REG32;  /* BL602 hardware TRNG */
}

uint8_t *EspClass::random(uint8_t *resultArray, const size_t outputSizeBytes)
{
    if (!resultArray) return 0;
    uint32_t tmp;
    for (size_t i = 0; i < outputSizeBytes; i++)
    {
        if (i % 4 == 0)
        {
            tmp = RANDOM_REG32;
        }
        resultArray[i] = (uint8_t)(tmp & 0xFF);
        tmp >>= 8;
    }
    return resultArray;
}
uint8_t  EspClass::getCpuFreqMHz(void)    { return (uint8_t)getCpuFrequencyMhz(); }

const char *EspClass::getSdkVersion(void) { return "bl602-sdk"; }

String EspClass::getCoreVersion(void) { return "0.1.0"; }

String EspClass::getFullVersion(void)
{
    String s;
    s.concat(F("SDK:"));
    s.concat(getSdkVersion());
    s.concat(F(" core "));
    s.concat(getCoreVersion());
    return s;
}

String EspClass::getResetReason(void)
{
    /* BL602 does not expose a public reset-cause register through the SDK;
     * report power-on. Refine when a library needs the real cause. */
    return F("Power-on reset");
}

String EspClass::getResetInfo(void)
{
    String s;
    s.concat(F("reason: "));
    s.concat(getResetReason());
    return s;
}

struct rst_info *EspClass::getResetInfoPtr(void) { return &resetInfo; }

void EspClass::restart(void) { esp_restart(); }
void EspClass::reset(void)   { esp_restart(); }

void EspClass::deepSleep(uint64_t time_us, RFMode mode)
{
    (void)mode;
    /* BL602 HBN deep sleep is not exposed by this SDK. Emulate the observable
     * behavior (system paused ~time_us, then continues) with delay() so the
     * caller keeps working afterwards. Real low-power sleep is a future
     * extension. */
    uint64_t remaining_ms = time_us / 1000;
    while (remaining_ms > 0) {
        uint64_t chunk = remaining_ms > 1000 ? 1000 : remaining_ms;
        delay((unsigned long)chunk);
        remaining_ms -= chunk;
    }
}

void EspClass::deepSleepInstant(uint64_t time_us, RFMode mode)
{
    deepSleep(time_us, mode);
}

/* ---- legacy NON-OS-SDK RTC-user-memory + OS-timer globals (LowPowerDemo) ----
 * RTC_USER_MEM is the ESP8266 user RTC RAM area. BL602 exposes no retained-RAM
 * address via this SDK, so it maps to a plain buffer: sketches that stash state
 * there still compile and run, it just doesn't survive a real reset. */
uint8_t s_rtc_user_mem[512] __attribute__((aligned(4)));

/* os_timer_t* timer_list — LowPowerDemo nulls this to stop the OS timers. */
os_timer_t* timer_list = NULL;

void EspClass::wdtEnable(uint32_t timeout_ms) { watchdogEnable(timeout_ms); }
void EspClass::wdtDisable(void) { watchdogDisable(); }
void EspClass::wdtFeed(void)    { watchdogFeed(); }

uint32_t EspClass::getFlashChipId(void)     { return esp_get_chip_id(); }
uint32_t EspClass::getFlashChipRealSize(void) { return 0x200000; } /* 2 MB */
uint32_t EspClass::getFlashChipSize(void)     { return 0x200000; }
uint32_t EspClass::getFlashChipSpeed(void)    { return 96000000; } /* BFLB FCLK */
FlashMode_t EspClass::getFlashChipMode(void)  { return FM_QIO; }
uint32_t EspClass::getSketchSize(void)        { return 0; }
uint32_t EspClass::getFreeSketchSpace(void)   { return 0x200000; }
bool EspClass::eraseConfig(void)              { return false; }

/* MD5 of the application region [0, 1 MiB) of flash (see Esp.h). Read in
 * sector-sized chunks so the flash driver's page windows are respected. */
String EspClass::getSketchMD5(void)
{
    MD5Builder md5;
    uint8_t buf[SPI_FLASH_SECTOR_SIZE];
    uint32_t addr;
    size_t chunk = sizeof(buf);

    md5.begin();
    for (addr = 0; addr < 0x100000; addr += chunk) {
        size_t n = (0x100000 - addr < chunk) ? (0x100000 - addr) : chunk;
        if (bl_flash_read(addr, buf, (int)n) != 0) {
            return emptyString;
        }
        md5.add(buf, n);
    }
    md5.calculate();
    return md5.toString();
}

/* Image-header helpers used by the Updater. BL602 has no ESP8266 0xE9 header,
 * so these report the fixed 2 MB QIO geometry — the Update state machine then
 * treats the incoming image as valid and the write proceeds. */
uint32_t EspClass::magicFlashChipSize(uint8_t)  { return 0x200000; }
FlashMode_t EspClass::magicFlashChipMode(uint8_t) { return FM_QIO; }
bool EspClass::checkFlashConfig(bool)           { return true; }

/* Direct flash access via the hosal layer. BL602 flash must be erased before
 * write; the Updater does exactly that (flashEraseSector then flashWrite). */
bool EspClass::flashEraseSector(uint32_t sector)
{
    return bl_flash_erase(sector * SPI_FLASH_SEC_SIZE, SPI_FLASH_SEC_SIZE) == 0;
}
bool EspClass::flashWrite(uint32_t address, const uint32_t *data, size_t size)
{
    return bl_flash_write(address, (uint8_t *)data, (int)size) == 0;
}
bool EspClass::flashWrite(uint32_t address, const uint8_t *data, size_t size)
{
    return bl_flash_write(address, (uint8_t *)data, (int)size) == 0;
}
bool EspClass::flashRead(uint32_t address, uint32_t *data, size_t size)
{
    return bl_flash_read(address, (uint8_t *)data, (int)size) == 0;
}
bool EspClass::flashRead(uint32_t address, uint8_t *data, size_t size)
{
    return bl_flash_read(address, data, (int)size) == 0;
}

/* RTC user memory: no RTC RAM on BL602 — a static SRAM region stands in, so
 * ESP8266 sketches that stash data before a soft reboot still compile and keep
 * state within a session. Size/offset semantics mirror the ESP8266 core
 * (offset in bytes, 4-byte aligned, 512-byte total). */
#define RTC_USER_MEM_SIZE 512
static uint8_t rtc_user_mem[RTC_USER_MEM_SIZE];

bool EspClass::rtcUserMemoryRead(uint32_t offset, uint32_t *data, size_t size)
{
    if (!data || size == 0 || offset + size > RTC_USER_MEM_SIZE || (offset & 3) || (size & 3))
        return false;
    memcpy(data, rtc_user_mem + offset, size);
    return true;
}

bool EspClass::rtcUserMemoryWrite(uint32_t offset, uint32_t *data, size_t size)
{
    if (!data || size == 0 || offset + size > RTC_USER_MEM_SIZE || (offset & 3) || (size & 3))
        return false;
    memcpy(rtc_user_mem + offset, data, size);
    return true;
}

/* The BL602 SDK exposes no ROM-download jump; a plain reboot is the closest
 * safe behaviour (matches ESP8266's [[noreturn]] contract). */
void EspClass::rebootIntoUartDownloadMode(void) { esp_restart(); }

uint16_t EspClass::getMaxFreeBlockSize(void)
{
    uint32_t free = esp_get_free_heap_size();
    return (free > 0xFFFF) ? 0xFFFF : (uint16_t)free;
}

void EspClass::getHeapStats(uint32_t* free, uint16_t* max, uint8_t* frag)
{
    if (free) *free = esp_get_free_heap_size();
    if (max)  *max  = getMaxFreeBlockSize();
    if (frag) *frag = 0; /* no heap fragmentation data on BL602 */
}

void EspClass::getHeapStats(uint32_t* free, uint32_t* max, uint8_t* frag)
{
    if (free) *free = esp_get_free_heap_size();
    if (max)  *max  = (uint32_t)getMaxFreeBlockSize();
    if (frag) *frag = 0;
}

void EspClass::setIramHeap(void)      { /* no-op: single newlib heap */ }
void EspClass::setExternalHeap(void)  { /* no-op */ }
void EspClass::resetHeap(void)        { /* no-op */ }

/* ESP8266 CRC-check of the sketch area in flash. The BL602 flash layout is
 * partition-based with no 0x40200000 sketch region, so report OK (the actual
 * app image is checked by the ROM/bootloader on every reset). */
bool EspClass::checkFlashCRC(void)    { return true; }

EspClass ESP;

/* Rate-limited yield, like the ESP8266 core: only actually yields once the
 * given interval has elapsed since the previous yield. SdFat calls this
 * between long SD transfers so the watchdog/network keep their share of CPU. */
void optimistic_yield(uint32_t interval_us)
{
    static uint32_t last_cycles = 0;
    uint32_t now = ESP.getCycleCount();
    uint32_t intvl_cycles = interval_us * ESP.getCpuFreqMHz();

    if (last_cycles == 0) {
        last_cycles = now;
        return;
    }
    if ((now - last_cycles) >= intvl_cycles) {
        last_cycles = now;
        yield();
    }
}
