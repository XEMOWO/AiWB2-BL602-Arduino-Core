/*
 * SD.h - ESP8266-compatible SD card library for the Ai-WB2-12F (BL602).
 *
 * Compile-compatible port. The WB2 board has no SD-card slot, so `SD` backs
 * onto the on-chip LittleFS "media" partition: begin() ignores the CS pin and
 * mounts LittleFS, and every File/FS operation (open/read/write/mkdir/...) is
 * the real LittleFS one. SD examples therefore compile unchanged and run
 * against on-chip storage instead of a card.
 *
 * The public API mirrors the ESP8266 core's SD.h (libraries/SD): there
 * FILE_READ/FILE_WRITE come from SdFat as uint8_t flags, and SDClass owns the
 * open() overloads that map them to string modes. SDClassFileMode() is the
 * same mapper as the reference; "w+" etc. are handled by FS.cpp's sflags().
 */

#ifndef __SD_H__
#define __SD_H__

#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>

using namespace fs;

/* LittleFS.cpp defines FS_MAX_OPEN_FILES / FS_PHYS_PAGE / FS_PHYS_BLOCK locally
 * for its own global construction; SD needs the same media-partition geometry,
 * so mirror the values here (see libraries/LittleFS/src/LittleFS.cpp). */
#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 5
#endif
#define FS_PHYS_PAGE  256
#define FS_PHYS_BLOCK 4096

/* The reference ESP8266 SD.h reads the O_* flags from SdFat; this toolchain's
 * newlib fcntl.h defines none of them, so supply POSIX-standard values here. */
#ifndef O_RDONLY
#define O_RDONLY 0
#endif
#ifndef O_WRONLY
#define O_WRONLY 1
#endif
#ifndef O_RDWR
#define O_RDWR   2
#endif
#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif
#ifndef O_APPEND
#define O_APPEND 0x0008
#endif

// Avoid type ambiguity, force u8 instead of untyped literal
// (ref. #6106 as to why we add APPEND to WRITE)
inline constexpr uint8_t SDClassFileRead { O_RDONLY };
#undef FILE_READ
#define FILE_READ SDClassFileRead

inline constexpr uint8_t SDClassFileWrite { O_RDWR | O_APPEND };
#undef FILE_WRITE
#define FILE_WRITE SDClassFileWrite

static inline const char* SDClassFileMode(uint8_t mode) {
    bool read = false;
    bool write = false;

    switch (mode & O_ACCMODE) {
    case O_RDONLY:
        read = true;
        break;
    case O_WRONLY:
        write = true;
        break;
    case O_RDWR:
        read = true;
        write = true;
        break;
    }

    const bool append = (mode & O_APPEND) > 0;

    if      (  read && !write )            { return "r";  }
    else if ( !read &&  write && !append ) { return "w+"; }
    else if ( !read &&  write &&  append ) { return "a";  }
    else if (  read &&  write && !append ) { return "w+"; } // may be a bug in FS::mode interpretation, "r+" seems proper
    else if (  read &&  write &&  append ) { return "a+"; }

    return "r";
}

class SDClass : public FS
{
public:
    SDClass()
        : FS(FSImplPtr(new littlefs_impl::LittleFSImpl(0, 0,
                FS_PHYS_PAGE, FS_PHYS_BLOCK, FS_MAX_OPEN_FILES)))
    {
    }

    bool begin(uint8_t csPin, uint32_t cfg = 4000000) // SPI_HALF_SPEED
    {
        (void)csPin; (void)cfg;
        return FS::begin();
    }

    void end(bool endSPI = true)
    {
        (void)endSPI;
        FS::end();
    }

    fs::File open(const char *filename, uint8_t mode = FILE_READ)
    {
        return FS::open(filename, SDClassFileMode(mode));
    }

    fs::File open(const char *filename, const char *mode)
    {
        return FS::open(filename, mode);
    }

    fs::File open(const String &filename, uint8_t mode = FILE_READ)
    {
        return open(filename.c_str(), mode);
    }

    fs::File open(const String &filename, const char *mode)
    {
        return open(filename.c_str(), mode);
    }
};

/* ---- Sd2Card: minimal shim for SdFat-style sketches (TFT shield examples) ----
 * The Seeed TFT examples do `Sd2Card card; card.init(SPI_FULL_SPEED, cs);`
 * before SD.begin(). On WB2 the media is on-chip LittleFS, so init() is a
 * no-op success. */
class Sd2Card
{
public:
    bool init(uint32_t sckRateID, uint8_t csPin) { (void)sckRateID; (void)csPin; return true; }
    void setSS(uint8_t ss) { (void)ss; }
    uint8_t readBlock(uint32_t block, uint8_t *dst) { (void)block; (void)dst; return 0; }
    uint8_t writeBlock(uint32_t block, const uint8_t *src) { (void)block; (void)src; return 0; }
    uint32_t cardSize() { return 0; }
};

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_SD)
extern SDClass SD;
#endif

#endif /* __SD_H__ */
