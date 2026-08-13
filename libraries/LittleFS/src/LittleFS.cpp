/*
 LittleFS.cpp - Wrapper for LittleFS for ESP8266
 Copyright (c_ 2019 Earle F. Philhower, III.  All rights reserved.

 Based extensively off of the ESP8266 SPIFFS code, which is
 Copyright (c) 2015 Ivan Grokhotkov. All rights reserved.
 This file is part of the esp8266 core for Arduino environment.

 Ported to Ai-WB2-12F (BL602): the backing region is the "media" partition,
 resolved lazily via bl_mtd on first begin()/format() (see wb2_littlefs_region
 below) instead of the ESP8266 linker symbols; the global LittleFS is built
 with size 0 and fills in geometry at begin().
 */

#include <Arduino.h>
#include <stdlib.h>
#include <algorithm>
#include "LittleFS.h"
#include "debug.h"
#include "flash_hal.h"

extern "C" {
#include "c_types.h"
#include "spi_flash.h"
#include <bl_mtd.h> /* C API, no extern "C" guards of its own */
}

namespace littlefs_impl {

/*
 * Resolve the LittleFS backing region from the boot2 partition table. The
 * "media" partition (BL_MTD_PARTITION_NAME_ROMFS) is the same dedicated app-
 * data area SPIFFS uses (0x1A2000 / 0x47000 in the 2M layout). Called lazily
 * so static construction of the global `LittleFS` never touches flash.
 */
void wb2_littlefs_region(uint32_t *start, uint32_t *size)
{
    static bool resolved = false;
    static uint32_t reg_start = 0, reg_size = 0;

    if (!resolved) {
        bl_mtd_handle_t h = NULL;
        if (bl_mtd_open(BL_MTD_PARTITION_NAME_ROMFS, &h, BL_MTD_OPEN_FLAG_NONE) == 0) {
            bl_mtd_info_t info;
            bl_mtd_info(h, &info);
            reg_start = info.offset;
            reg_size  = info.size;
            bl_mtd_close(h);
        }
        resolved = true;
    }
    *start = reg_start;
    *size  = reg_size;
}

FileImplPtr LittleFSImpl::open(const char* path, OpenMode openMode, AccessMode accessMode) {
    if (!_mounted) {
        DEBUGV("LittleFSImpl::open() called on unmounted FS\n");
        return FileImplPtr();
    }
    if (!path || !path[0]) {
        DEBUGV("LittleFSImpl::open() called with invalid filename\n");
        return FileImplPtr();
    }
    if (!LittleFSImpl::pathValid(path)) {
        DEBUGV("LittleFSImpl::open() called with too long filename\n");
        return FileImplPtr();
    }

    int flags = _getFlags(openMode, accessMode);
    auto fd = std::make_shared<lfs_file_t>();

    if ((openMode & OM_CREATE) && strchr(path, '/')) {
        // For file creation, silently make subdirs as needed.  If any fail,
        // it will be caught by the real file open later on
        char *pathStr = strdup(path);
        if (pathStr) {
            // Make dirs up to the final fnamepart
            char *ptr = strchr(pathStr, '/');
            while (ptr) {
                *ptr = 0;
                lfs_mkdir(&_lfs, pathStr);
                *ptr = '/';
                ptr = strchr(ptr+1, '/');
            }
        }
        free(pathStr);
    }

    time_t creation = 0;
    if (_timeCallback && (openMode & OM_CREATE)) {
        // O_CREATE means we *may* make the file, but not if it already exists.
        // See if it exists, and only if not update the creation time
        int rc = lfs_file_open(&_lfs, fd.get(), path, LFS_O_RDONLY);
	if (rc == 0) {
            lfs_file_close(&_lfs, fd.get()); // It exists, don't update create time
        } else {
            creation = _timeCallback();  // File didn't exist or otherwise, so we're going to create this time
        }
    }

    int rc = lfs_file_open(&_lfs, fd.get(), path, flags);
    if (rc == LFS_ERR_ISDIR) {
        // To support the SD.openNextFile, a null FD indicates to the LittleFSFile this is just
        // a directory whose name we are carrying around but which cannot be read or written
        return std::make_shared<LittleFSFileImpl>(this, path, nullptr, flags, creation);
    } else if (rc == 0) {
        lfs_file_sync(&_lfs, fd.get());
        return std::make_shared<LittleFSFileImpl>(this, path, fd, flags, creation);
    } else {
        DEBUGV("LittleFSDirImpl::openFile: rc=%d fd=%p path=`%s` openMode=%d accessMode=%d err=%d\n",
               rc, fd.get(), path, openMode, accessMode, rc);
        return FileImplPtr();
    }
}

DirImplPtr LittleFSImpl::openDir(const char *path) {
    if (!_mounted || !path) {
        return DirImplPtr();
    }
    char *pathStr = strdup(path); // Allow edits on our scratch copy
    // Get rid of any trailing slashes
    while (strlen(pathStr) && (pathStr[strlen(pathStr)-1]=='/')) {
        pathStr[strlen(pathStr)-1] = 0;
    }
    // At this point we have a name of "blah/blah/blah" or "blah" or ""
    // If that references a directory, just open it and we're done.
    lfs_info info;
    auto dir = std::make_shared<lfs_dir_t>();
    int rc;
    const char *filter = "";
    if (!pathStr[0]) {
        // openDir("") === openDir("/")
        rc = lfs_dir_open(&_lfs, dir.get(), "/");
        filter = "";
    } else if (lfs_stat(&_lfs, pathStr, &info) >= 0) {
        if (info.type == LFS_TYPE_DIR) {
            // Easy peasy, path specifies an existing dir!
            rc = lfs_dir_open(&_lfs, dir.get(), pathStr);
	    filter = "";
        } else {
            // This is a file, so open the containing dir
            char *ptr = strrchr(pathStr, '/');
            if (!ptr) {
                // No slashes, open the root dir
                rc = lfs_dir_open(&_lfs, dir.get(), "/");
		filter = pathStr;
            } else {
                // We've got slashes, open the dir one up
                *ptr = 0; // Remove slash, truncate string
                rc = lfs_dir_open(&_lfs, dir.get(), pathStr);
		filter = ptr + 1;
            }
        }
    } else {
        // Name doesn't exist, so use the parent dir of whatever was sent in
        // This is a file, so open the containing dir
        char *ptr = strrchr(pathStr, '/');
        if (!ptr) {
            // No slashes, open the root dir
            rc = lfs_dir_open(&_lfs, dir.get(), "/");
	    filter = pathStr;
        } else {
            // We've got slashes, open the dir one up
            *ptr = 0; // Remove slash, truncate string
            rc = lfs_dir_open(&_lfs, dir.get(), pathStr);
	    filter = ptr + 1;
        }
    }
    if (rc < 0) {
        DEBUGV("LittleFSImpl::openDir: path=`%s` err=%d\n", path, rc);
        free(pathStr);
        return DirImplPtr();
    }
    // Skip the . and .. entries
    lfs_info dirent;
    lfs_dir_read(&_lfs, dir.get(), &dirent);
    lfs_dir_read(&_lfs, dir.get(), &dirent);

    auto ret = std::make_shared<LittleFSDirImpl>(filter, this, dir, pathStr);
    free(pathStr);
    return ret;
}

int LittleFSImpl::lfs_flash_read(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, void *dst, lfs_size_t size) {
    LittleFSImpl *me = reinterpret_cast<LittleFSImpl*>(c->context);
    uint32_t addr = me->_start + (block * me->_blockSize) + off;
    return flash_hal_read(addr, size, static_cast<uint8_t*>(dst)) == FLASH_HAL_OK ? 0 : -1;
}

int LittleFSImpl::lfs_flash_prog(const struct lfs_config *c,
    lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    LittleFSImpl *me = reinterpret_cast<LittleFSImpl*>(c->context);
    uint32_t addr = me->_start + (block * me->_blockSize) + off;
    const uint8_t *src = reinterpret_cast<const uint8_t *>(buffer);
    return flash_hal_write(addr, size, static_cast<const uint8_t*>(src)) == FLASH_HAL_OK ? 0 : -1;
}

int LittleFSImpl::lfs_flash_erase(const struct lfs_config *c, lfs_block_t block) {
    LittleFSImpl *me = reinterpret_cast<LittleFSImpl*>(c->context);
    uint32_t addr = me->_start + (block * me->_blockSize);
    uint32_t size = me->_blockSize;
    return flash_hal_erase(addr, size) == FLASH_HAL_OK ? 0 : -1;
}

int LittleFSImpl::lfs_flash_sync(const struct lfs_config *c) {
    /* NOOP */
    (void) c;
    return 0;
}


}; // namespace

// these symbols should be defined in the linker script for each flash layout
// (The reference guards this with #ifndef CORE_MOCK, but our build defines
// CORE_MOCK=1 to let sketches take their portable paths, so the global LittleFS
// is always emitted here.)
#ifdef ARDUINO
#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 5
#endif

#if !defined(NO_GLOBAL_INSTANCES) && !defined(NO_GLOBAL_LITTLEFS)
/* Start/size are 0: the "media" partition geometry is resolved lazily in
 * begin() (see wb2_littlefs_region). Page/block are the BL602 flash sector. */
#define FS_PHYS_PAGE  256
#define FS_PHYS_BLOCK 4096
FS LittleFS = FS(FSImplPtr(new littlefs_impl::LittleFSImpl(0, 0, FS_PHYS_PAGE, FS_PHYS_BLOCK, FS_MAX_OPEN_FILES)));

extern "C" void littlefs_request_end(void)
{
    // override default weak function
    //ets_printf("debug: not weak littlefs end\n");
    LittleFS.end();
}

#endif
#endif
