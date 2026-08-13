/*
 * Esp.h — ESP8266-compatible `ESP` class for the Ai-WB2-12F (BL602) core.
 *
 * Implements the subset of the official ESP8266 EspClass API that third-party
 * libraries and sketches actually call (getChipId/getFreeHeap/getCycleCount/
 * getResetReason/restart/wdt/deepSleep/flash introspection). The goal is that
 * ESP8266 code "拿来就能编能跑" - API compatibility, not bit-exact emulation.
 * Where the BL602 SDK exposes no hardware feature, the method degrades to a
 * safe, documented behavior (see each member).
 *
 * Official ESP8266 reference: cores/esp8266/Esp.h in the esp8266/Arduino repo.
 */
#ifndef ESP_H
#define ESP_H

#include <Arduino.h>
/* struct rst_info / REASON_* reset-cause enum — the ESP8266 core's Esp.h pulls
 * user_interface.h for these; DNSServer reads `resetInfo.reason` directly. */
#include "user_interface.h"

/* Hardware random word. ESP8266 exposes RANDOM_REG32 as the Xtensa RNG
 * register; BL602's equivalent is the secure-engine TRNG, read via
 * bl_sec_get_random_word() (declared here rather than pulling bl_sec.h so
 * C++ TUs that only need the macro don't drag in the SDK header chain). */
extern "C" uint32_t bl_sec_get_random_word(void);
#define RANDOM_REG32  bl_sec_get_random_word()

/* Watchdog timeout constants — same names/values as the ESP8266 core. */
typedef enum {
    WDTO_0MS    = 0,
    WDTO_15MS   = 15,
    WDTO_30MS   = 30,
    WDTO_60MS   = 60,
    WDTO_120MS  = 120,
    WDTO_250MS  = 250,
    WDTO_500MS  = 500,
    WDTO_1S     = 1000,
    WDTO_2S     = 2000,
    WDTO_4S     = 4000,
    WDTO_8S     = 8000
} WDTO_t;

#define wdt_enable(time)    ESP.wdtEnable(time)
#define wdt_disable()       ESP.wdtDisable()
#define wdt_reset()         ESP.wdtFeed()

/* IRQ lock/unlock — ESP8266 spells these cli/sei via ets_intr_lock/unlock. */
#define cli()        ets_intr_lock()
#define sei()        ets_intr_unlock()

enum RFMode {
    RF_DEFAULT = 0,
    RF_CAL     = 1,
    RF_NO_CAL  = 2,
    RF_DISABLED = 4
};
#define WakeMode RFMode
#define WAKE_RF_DEFAULT  RF_DEFAULT
#define WAKE_RFCAL       RF_CAL
#define WAKE_NO_RFCAL    RF_NO_CAL
#define WAKE_RF_DISABLED RF_DISABLED

/* ADC mode markers (ADC_MODE(ADC_VCC) in sketches). BL602 has a single ADC
 * channel with no VCC/TOUT split, so __get_adc_mode() is just a stub. */
enum ADCMode {
    ADC_TOUT    = 33,
    ADC_TOUT_3V3 = 33,
    ADC_VCC     = 255,
    ADC_VDD     = 255
};
#define ADC_MODE(mode) int __get_adc_mode(void) { return (int)(mode); }

typedef enum {
    FM_QIO     = 0x00,
    FM_QOUT    = 0x01,
    FM_DIO     = 0x02,
    FM_DOUT    = 0x03,
    FM_UNKNOWN = 0xff
} FlashMode_t;

class EspClass {
    public:
        /* Cycle counter — RISC-V `cycle` CSR (increments at 192 MHz). Same
         * 32-bit wrap semantics as ESP8266's ccount; used by DHT, NeoPixel
         * (via _getCycleCount) and other timing libraries. */
        static inline uint32_t getCycleCount() __attribute__((always_inline))
        {
            uint32_t cnt;
            __asm__ volatile ("csrr %0, cycle" : "=r"(cnt));
            return cnt;
        }

        /* ESP8266-compatible random bytes. Backed by the hardware TRNG
         * (RANDOM_REG32 → bl_sec_get_random_word), matching ESP8266's use of
         * its RNG register. WiFiMesh (session-key generation) and hash-based
         * sketches rely on these. */
        static uint8_t *random(uint8_t *resultArray, const size_t outputSizeBytes);
        static uint32_t random(void);

        static uint32_t getFreeHeap(void);
        static uint32_t getFreeContStack(void);
        /* ESP8266 reads its internal ADC to report the module VCC in mV. BL602
         * exposes no equivalent here; report the nominal 3.3 V supply. */
        static uint32_t getVcc(void);
        static void resetFreeContStack(void);  /* SdFat calls this before a big stack push */

        static uint32_t getChipId(void);
        static uint8_t  getCpuFreqMHz(void);

        static const char *getSdkVersion(void);
        static String getCoreVersion(void);
        static String getFullVersion(void);

        static String getResetReason(void);
        static String getResetInfo(void);
        static struct rst_info *getResetInfoPtr(void);

        static void restart(void);
        static void reset(void);
        static void deepSleep(uint64_t time_us, RFMode mode = RF_DEFAULT);
        /* ESP8266 deepSleepInstant(): enter deep sleep without RF calibration
         * dance. BL602 deep sleep isn't exposed, so same delay() emulation. */
        static void deepSleepInstant(uint64_t time_us, RFMode mode = RF_DEFAULT);

        static void wdtEnable(uint32_t timeout_ms = 0);
        static void wdtDisable(void);
        static void wdtFeed(void);

        static uint32_t getFlashChipId(void);
        static uint32_t getFlashChipRealSize(void);
        static uint32_t getFlashChipSize(void);
        static uint32_t getFlashChipSpeed(void);
        static FlashMode_t getFlashChipMode(void);

        static uint32_t getSketchSize(void);
        static uint32_t getFreeSketchSpace(void);

        /* MD5 of the currently-running firmware image, as a 32-char hex string
         * (ESP8266: hash of the sketch flash region; used in the
         * x-ESP8266-sketch-md5 OTA header). BL602 has no separate sketch
         * partition visible to user code, so this hashes the application
         * region [0, 1 MiB) of flash. */
        static String getSketchMD5(void);

        /* ESP8266-core image-header helpers (Updater/OTA). BL602 firmware has
         * no ESP8266-style 0xE9 header, so these report the chip's fixed
         * geometry — sufficient for the Update state machine to run. */
        static uint32_t magicFlashChipSize(uint8_t byte);
        static FlashMode_t magicFlashChipMode(uint8_t byte);
        static bool checkFlashConfig(bool needsEquals = false);

        /* Direct flash access on top of the hosal layer (bl_flash_*). BL602
         * writes need an erased destination, so flashEraseSector is normally
         * called before flashWrite — exactly the Updater's pattern. */
        static bool flashEraseSector(uint32_t sector);
        static bool flashWrite(uint32_t address, const uint32_t *data, size_t size);
        static bool flashWrite(uint32_t address, const uint8_t *data, size_t size);
        static bool flashRead(uint32_t address, uint32_t *data, size_t size);
        static bool flashRead(uint32_t address, uint8_t *data, size_t size);

        static bool eraseConfig(void);
        static bool checkFlashCRC(void);

        /* RTC user-memory API (ESP8266: 512 B of RTC RAM). BL602 has no RTC
         * RAM exposed to user code; backed by a static SRAM region so sketches
         * (WiFiShutdown, RTCUserMemory) still compile and hold state across
         * soft restarts. */
        static bool rtcUserMemoryRead(uint32_t offset, uint32_t *data, size_t size);
        static bool rtcUserMemoryWrite(uint32_t offset, uint32_t *data, size_t size);

        /* ESP8266: reboot straight into ROM download mode. No such primitive is
         * exposed by the BL602 SDK, so this performs a normal reboot. */
        [[noreturn]] static void rebootIntoUartDownloadMode(void);

        /* Heap introspection used by the umm_malloc examples (HeapMetric,
         * irammem, MMU48K). Signature matches ESP8266 3.0.2 Esp.h. */
        static uint16_t getMaxFreeBlockSize(void);
        static void getHeapStats(uint32_t* free = nullptr, uint16_t* max = nullptr,
                                 uint8_t* frag = nullptr);
        /* ESP8266 3.0.2 added a uint32_t* max overload (deprecating the one
         * above); HeapMetric/irammem pass uint32_t `max`, so both must exist. */
        static void getHeapStats(uint32_t* free, uint32_t* max, uint8_t* frag);

        /* ESP8266 virtual-memory heap selectors. No BL602 equivalent — the
         * BL602 uses a single newlib heap, so these are no-ops (they exist so
         * code that rearranges heaps still compiles). */
        static void setIramHeap(void);
        static void setExternalHeap(void);
        static void resetHeap(void);
};

extern EspClass ESP;

#endif /* ESP_H */
