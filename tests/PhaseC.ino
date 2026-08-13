/*
 * PhaseC.ino — Phase C acceptance sketch: FS/SPIFFS + EEPROM + time.
 *
 * Exercises the ESP8266-compatible API surface of the Ai-WB2-12F core:
 *   - SPIFFS (format / open / write / read / exists / info / openDir / remove)
 *   - EEPROM (begin / read / write / put / get / operator[] / commit / end)
 *   - time   (time / settimeofday / configTime / setTZ / getLocalTime)
 *
 * Builds and links with tools/ide_sim_build.sh; run on hardware via the
 * bflb_iot_tool flash script for the live checks.
 */
#include <FS.h>
#include <EEPROM.h>
#include <time.h>
#include <sys/time.h>

void setup(void)
{
    Serial.begin(115200);
    delay(100);

    Serial.println();
    Serial.println("== Phase C: FS + EEPROM + time ==");

    /* ---------------- SPIFFS ---------------- */
    if (!SPIFFS.begin()) {
        Serial.println("SPIFFS: mount failed, formatting");
        SPIFFS.format();
        if (!SPIFFS.begin()) {
            Serial.println("SPIFFS: mount failed after format");
        }
    }

    FSInfo fi;
    if (SPIFFS.info(fi)) {
        Serial.printf("SPIFFS: total=%u used=%u block=%u page=%u maxOpen=%u\n",
                      (unsigned) fi.totalBytes, (unsigned) fi.usedBytes,
                      (unsigned) fi.blockSize, (unsigned) fi.pageSize,
                      (unsigned) fi.maxOpenFiles);
    }

    File f = SPIFFS.open("/test.txt", "w");
    if (f) {
        f.print("Hello WB2 Phase C");
        f.write((uint8_t) '!');
        f.println(" / 100% ESP8266 compat");
        f.close();
        Serial.println("SPIFFS: wrote /test.txt");
    } else {
        Serial.println("SPIFFS: open(w) failed");
    }

    Serial.printf("SPIFFS: exists(/test.txt)=%d\n", SPIFFS.exists("/test.txt"));

    f = SPIFFS.open("/test.txt", "r");
    if (f) {
        Serial.printf("SPIFFS: read %u bytes: \"", (unsigned) f.size());
        while (f.available()) {
            Serial.write(f.read());
        }
        Serial.println("\"");
        f.close();
    }

    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
        Serial.printf("SPIFFS: dir entry %s (%u bytes)\n",
                      dir.fileName().c_str(), (unsigned) dir.fileSize());
    }

    Serial.printf("SPIFFS: remove=%d, exists-after=%d\n",
                  SPIFFS.remove("/test.txt"), SPIFFS.exists("/test.txt"));

    /* ---------------- EEPROM ---------------- */
    EEPROM.begin(256);
    Serial.printf("EEPROM: length=%u, fresh[0]=%02X (FF => never used)\n",
                  (unsigned) EEPROM.length(), EEPROM.read(0));

    EEPROM.write(0, 0xAA);
    EEPROM.write(1, 0x55);
    EEPROM.put(2, (uint32_t) 0x12345678UL);
    EEPROM[10] = 0x77;                     /* operator[] marks dirty */
    Serial.printf("EEPROM: commit=%d\n", EEPROM.commit());

    uint32_t v = 0;
    EEPROM.get(2, v);
    Serial.printf("EEPROM: [0]=%02X [1]=%02X [10]=%02X put/get=%08X\n",
                  EEPROM.read(0), EEPROM.read(1), EEPROM.read(10), (unsigned) v);

    Serial.printf("EEPROM: end=%d\n", EEPROM.end());

    /* ---------------- time ---------------- */
    time_t now = time(nullptr);
    Serial.printf("time(): boot-epoch=%ld\n", (long) now);

    struct timeval tv;
    tv.tv_sec = 1700000000;
    tv.tv_usec = 500000;
    Serial.printf("settimeofday rc=%d\n", settimeofday(&tv, nullptr));
    Serial.printf("time() after settimeofday=%ld\n", (long) time(nullptr));

    configTime(8 * 3600, 0, "pool.ntp.org", "time.nist.gov", nullptr);
    setTZ("CST-8");
    configTzTime("EST5EDT,M3.2.0,M11.1.0", "0.pool.ntp.org");

    struct tm tminfo;
    bool got = getLocalTime(&tminfo, 200); /* no NTP yet: returns false */
    Serial.printf("getLocalTime(200ms)=%d tm_year=%d\n", got, tminfo.tm_year);

    Serial.println("== Phase C done ==");
}

void loop(void)
{
    delay(1000);
    yield();
}
