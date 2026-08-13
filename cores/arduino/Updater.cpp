/*
 * Updater.cpp — ESP8266-core compatible OTA Update class for the BL602.
 *
 * Ported from cores/esp8266/Updater.cpp (3.0.2). The public API and the
 * buffer/MD5/verify state machine are kept verbatim so ESP8266httpUpdate,
 * ArduinoOTA and the WebUpdate examples compile and run identically up to the
 * flash-write layer. Platform differences:
 *
 *  - eboot_command / bootloader staging does not exist on BL602: a U_FLASH
 *    update is written directly to flash (staged in the free region just
 *    below the filesystem), and end() does not arm a two-stage reboot.
 *  - The boot-mode bootstrap check and the Xtensa XIP-base address math are
 *    replaced by BL602 flash-offset constants.
 *  - The first-image magic check (0xE9) is kept for source compatibility;
 *    a real BL602 OTA needs the partition map from the build config.
 */
#include "Updater.h"
#include <esp8266_peri.h>
#include <PolledTimeout.h>
#include "StackThunk.h"

#include <Updater_Signing.h>
#ifndef ARDUINO_SIGNING
  #define ARDUINO_SIGNING 0
#endif

extern "C" {
    #include "c_types.h"
    #include "spi_flash.h"
    #include "user_interface.h"
}

/* BL602 flash layout (raw offsets). boot2 sits at 0x0; the application image
 * follows. A U_FLASH update is staged in the free region at the top of flash
 * (below the filesystem), which is far from the running image's XIP window. */
#define WB2_FS_START      0x100000      /* 1 MiB filesystem base */
#define WB2_FLASH_SIZE    0x200000      /* 2 MiB total flash */

UpdaterClass::UpdaterClass()
{
#if ARDUINO_SIGNING
  installSignature(&esp8266::updaterSigningHash, &esp8266::updaterSigningVerifier);
  stack_thunk_add_ref();
#endif
}

UpdaterClass::~UpdaterClass()
{
#if ARDUINO_SIGNING
    stack_thunk_del_ref();
#endif
}

UpdaterClass& UpdaterClass::onProgress(THandlerFunction_Progress fn) {
    _progress_callback = fn;
    return *this;
}

void UpdaterClass::_reset() {
  if (_buffer)
    delete[] _buffer;
  _buffer = 0;
  _bufferLen = 0;
  _startAddress = 0;
  _currentAddress = 0;
  _size = 0;
  _command = U_FLASH;

  if(_ledPin != -1) {
    digitalWrite(_ledPin, !_ledOn); // off
  }
}

bool UpdaterClass::begin(size_t size, int command, int ledPin, uint8_t ledOn) {
  if(_size > 0){
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.println(F("[begin] already running"));
#endif
    return false;
  }

  _ledPin = ledPin;
  _ledOn = !!ledOn; // 0(LOW) or 1(HIGH)

#ifdef DEBUG_UPDATER
  if (command == U_FS) {
    DEBUG_UPDATER.println(F("[begin] Update Filesystem."));
  }
#endif

  if(size == 0) {
    _setError(UPDATE_ERROR_SIZE);
    return false;
  }

  if(!ESP.checkFlashConfig(false)) {
    _setError(UPDATE_ERROR_FLASH_CONFIG);
    return false;
  }

  _reset();
  clearError(); //  _error = 0
  _target_md5 = emptyString;
  _md5 = MD5Builder();

  wifi_set_sleep_type(NONE_SLEEP_T);

  //address where we will start writing the update
  uintptr_t updateStartAddress = 0;
  //size of current sketch rounded to a sector
  size_t currentSketchSize = (ESP.getSketchSize() + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));
  //size of the update rounded to a sector
  size_t roundedSize = (size + FLASH_SECTOR_SIZE - 1) & (~(FLASH_SECTOR_SIZE - 1));

  if (command == U_FLASH) {
    //stage the update in the free region at the top of flash
    updateStartAddress = (WB2_FLASH_SIZE > roundedSize)? (WB2_FLASH_SIZE - roundedSize) : 0;

#ifdef DEBUG_UPDATER
        DEBUG_UPDATER.printf_P(PSTR("[begin] roundedSize:       0x%08zX (%zd)\n"), roundedSize, roundedSize);
        DEBUG_UPDATER.printf_P(PSTR("[begin] updateEndAddress:  0x%08zX (%zd)\n"), updateStartAddress, updateStartAddress);
        DEBUG_UPDATER.printf_P(PSTR("[begin] currentSketchSize: 0x%08zX (%zd)\n"), currentSketchSize, currentSketchSize);
#endif

    //make sure that the size of both sketches is less than the total space
    if(updateStartAddress < currentSketchSize) {
      _setError(UPDATE_ERROR_SPACE);
      return false;
    }
  }
  else if (command == U_FS) {
    if(WB2_FS_START + roundedSize > WB2_FLASH_SIZE) {
      _setError(UPDATE_ERROR_SPACE);
      return false;
    }

    updateStartAddress = WB2_FS_START;
  }
  else {
    // unknown command
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.println(F("[begin] Unknown update command."));
#endif
    return false;
  }

  //initialize
  _startAddress = updateStartAddress;
  _currentAddress = _startAddress;
  _size = size;
  if (ESP.getFreeHeap() > 2 * FLASH_SECTOR_SIZE) {
    _bufferSize = FLASH_SECTOR_SIZE;
  } else {
    _bufferSize = 256;
  }
  _buffer = new uint8_t[_bufferSize];
  _command = command;

#ifdef DEBUG_UPDATER
  DEBUG_UPDATER.printf_P(PSTR("[begin] _startAddress:     0x%08X (%d)\n"), _startAddress, _startAddress);
  DEBUG_UPDATER.printf_P(PSTR("[begin] _currentAddress:   0x%08X (%d)\n"), _currentAddress, _currentAddress);
  DEBUG_UPDATER.printf_P(PSTR("[begin] _size:             0x%08zX (%zd)\n"), _size, _size);
#endif

  if (!_verify) {
    _md5.begin();
  }
  return true;
}

bool UpdaterClass::setMD5(const char * expected_md5){
  if(strlen(expected_md5) != 32)
  {
    return false;
  }
  _target_md5 = expected_md5;
  return true;
}

bool UpdaterClass::end(bool evenIfRemaining){
  if(_size == 0){
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.println(F("no update"));
#endif
    _reset();
    return false;
  }

  // Updating w/o any data is an error we detect here
  if (!progress()) {
    _setError(UPDATE_ERROR_NO_DATA);
  }

  if(hasError() || (!isFinished() && !evenIfRemaining)){
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.printf_P(PSTR("premature end: res:%u, pos:%zu/%zu\n"), getError(), progress(), _size);
#endif
    _reset();
    return false;
  }

  if(evenIfRemaining) {
    if(_bufferLen > 0) {
      _writeBuffer();
    }
    _size = progress();
  }

  uint32_t sigLen = 0;
  if (_verify) {
    ESP.flashRead(_startAddress + _size - sizeof(uint32_t), &sigLen, sizeof(uint32_t));
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.printf_P(PSTR("[Updater] sigLen: %d\n"), sigLen);
#endif
    if (sigLen != _verify->length()) {
      _setError(UPDATE_ERROR_SIGN);
      _reset();
      return false;
    }

    int binSize = _size - sigLen - sizeof(uint32_t) /* The siglen word */;
    _hash->begin();
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.printf_P(PSTR("[Updater] Adjusted binsize: %d\n"), binSize);
#endif
      // Calculate the MD5 and hash using proper size
    uint8_t buff[128] __attribute__((aligned(4)));
    for(int i = 0; i < binSize; i += sizeof(buff)) {
      ESP.flashRead(_startAddress + i, (uint32_t *)buff, sizeof(buff));
      size_t read = std::min((int)sizeof(buff), binSize - i);
      _hash->add(buff, read);
    }
    _hash->end();
#ifdef DEBUG_UPDATER
    unsigned char *ret = (unsigned char *)_hash->hash();
    DEBUG_UPDATER.printf_P(PSTR("[Updater] Computed Hash:"));
    for (int i=0; i<_hash->len(); i++) DEBUG_UPDATER.printf(" %02x", ret[i]);
    DEBUG_UPDATER.printf("\n");
#endif
    uint8_t *sig = (uint8_t*)malloc(sigLen);
    if (!sig) {
      _setError(UPDATE_ERROR_SIGN);
      _reset();
      return false;
    }
    ESP.flashRead(_startAddress + binSize, sig, sigLen);
#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.printf_P(PSTR("[Updater] Received Signature:"));
    for (size_t i=0; i<sigLen; i++) {
      DEBUG_UPDATER.printf(" %02x", sig[i]);
    }
    DEBUG_UPDATER.printf("\n");
#endif
    if (!_verify->verify(_hash, (void *)sig, sigLen)) {
      free(sig);
      _setError(UPDATE_ERROR_SIGN);
      _reset();
      return false;
    }
    free(sig);
    _size = binSize; // Adjust size to remove signature, not part of bin payload

#ifdef DEBUG_UPDATER
    DEBUG_UPDATER.printf_P(PSTR("[Updater] Signature matches\n"));
#endif
  } else if (_target_md5.length()) {
    _md5.calculate();
    if (strcasecmp(_target_md5.c_str(), _md5.toString().c_str())) {
      _setError(UPDATE_ERROR_MD5);
      return false;
    }
#ifdef DEBUG_UPDATER
    else DEBUG_UPDATER.printf_P(PSTR("MD5 Success: %s\n"), _target_md5.c_str());
#endif
  }

  if(!_verifyEnd()) {
    _reset();
    return false;
  }

  /* BL602 has no eboot staging: the bytes were already written to flash by
   * _writeBuffer(), so end() just validates and reports success. A real OTA
   * would additionally need the bootloader/partition map to switch images. */
#ifdef DEBUG_UPDATER
  DEBUG_UPDATER.printf_P(PSTR("Update staged: address:0x%08X, size:0x%08zX\n"), _startAddress, _size);
#endif

  _reset();
  return true;
}

bool UpdaterClass::_writeBuffer(){
  #define FLASH_MODE_PAGE  0
  #define FLASH_MODE_OFFSET  2

  bool eraseResult = true, writeResult = true;
  if (_currentAddress % FLASH_SECTOR_SIZE == 0) {
    if(!_async) yield();
    eraseResult = ESP.flashEraseSector(_currentAddress/FLASH_SECTOR_SIZE);
  }

  // If the flash settings don't match what we already have, modify them.
  // But restore them after the modification, so the hash isn't affected.
  // This is analogous to what esptool.py does when it receives a --flash_mode argument.
  bool modifyFlashMode = false;
  FlashMode_t flashMode = FM_QIO;
  FlashMode_t bufferFlashMode = FM_QIO;
  //TODO - GZIP can't do this
  if ((_currentAddress == _startAddress + FLASH_MODE_PAGE) && (_buffer[0] != 0x1f) && (_command == U_FLASH)) {
    flashMode = ESP.getFlashChipMode();
    #ifdef DEBUG_UPDATER
      DEBUG_UPDATER.printf_P(PSTR("Header: 0x%1X %1X %1X %1X\n"), _buffer[0], _buffer[1], _buffer[2], _buffer[3]);
    #endif
    bufferFlashMode = ESP.magicFlashChipMode(_buffer[FLASH_MODE_OFFSET]);
    if (bufferFlashMode != flashMode) {
      #ifdef DEBUG_UPDATER
        DEBUG_UPDATER.printf_P(PSTR("Set flash mode from 0x%1X to 0x%1X\n"), bufferFlashMode, flashMode);
      #endif

      _buffer[FLASH_MODE_OFFSET] = flashMode;
      modifyFlashMode = true;
    }
  }

  if (eraseResult) {
    if(!_async) yield();
    writeResult = ESP.flashWrite(_currentAddress, _buffer, _bufferLen);
  } else { // if erase was unsuccessful
    _currentAddress = (_startAddress + _size);
    _setError(UPDATE_ERROR_ERASE);
    return false;
  }

  // Restore the old flash mode, if we modified it.
  // Ensures that the MD5 hash will still match what was sent.
  if (modifyFlashMode) {
    _buffer[FLASH_MODE_OFFSET] = bufferFlashMode;
  }

  if (!writeResult) {
    _currentAddress = (_startAddress + _size);
    _setError(UPDATE_ERROR_WRITE);
    return false;
  }
  if (!_verify) {
    _md5.add(_buffer, _bufferLen);
  }
  _currentAddress += _bufferLen;
  _bufferLen = 0;
  return true;
}

size_t UpdaterClass::write(uint8_t *data, size_t len) {
  if(hasError() || !isRunning())
    return 0;

  if(progress() + _bufferLen + len > _size) {
    _setError(UPDATE_ERROR_SPACE);
    return 0;
  }

  size_t left = len;

  while((_bufferLen + left) > _bufferSize) {
    size_t toBuff = _bufferSize - _bufferLen;
    memcpy(_buffer + _bufferLen, data + (len - left), toBuff);
    _bufferLen += toBuff;
    if(!_writeBuffer()){
      return len - left;
    }
    left -= toBuff;
    if(!_async) yield();
  }
  //lets see what's left
  memcpy(_buffer + _bufferLen, data + (len - left), left);
  _bufferLen += left;
  if(_bufferLen == remaining()){
    //we are at the end of the update, so should write what's left to flash
    if(!_writeBuffer()){
      return len - left;
    }
  }
  return len;
}

bool UpdaterClass::_verifyHeader(uint8_t data) {
    if(_command == U_FLASH) {
        // check for valid first magic byte (is always 0xE9)
        if ((data != 0xE9) && (data != 0x1f)) {
            _currentAddress = (_startAddress + _size);
            _setError(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }
        return true;
    } else if(_command == U_FS) {
        // no check of FS possible with first byte.
        return true;
    }
    return false;
}

bool UpdaterClass::_verifyEnd() {
    if(_command == U_FLASH) {

        uint8_t buf[4] __attribute__((aligned(4)));
        if(!ESP.flashRead(_startAddress, (uint32_t *) &buf[0], 4)) {
            _currentAddress = (_startAddress);
            _setError(UPDATE_ERROR_READ);
            return false;
        }

        // check for valid first magic byte
        //
        // TODO: GZIP compresses the chipsize flags, so can't do check here
        if ((buf[0] == 0x1f) && (buf[1] == 0x8b)) {
            // GZIP, just assume OK
            return true;
        } else if (buf[0] != 0xE9) {
            _currentAddress = (_startAddress);
            _setError(UPDATE_ERROR_MAGIC_BYTE);
            return false;
        }

        uint32_t bin_flash_size = ESP.magicFlashChipSize((buf[3] & 0xf0) >> 4);

        // check if new bin fits to SPI flash
        if(bin_flash_size > ESP.getFlashChipRealSize()) {
            _currentAddress = (_startAddress);
            _setError(UPDATE_ERROR_NEW_FLASH_CONFIG);
            return false;
        }

        return true;
    } else if(_command == U_FS) {
        // FS is already over written checks make no sense any more.
        return true;
    }
    return false;
}

size_t UpdaterClass::writeStream(Stream &data, uint16_t streamTimeout) {
    size_t written = 0;
    size_t toRead = 0;
    if(hasError() || !isRunning())
        return 0;

    if(!_verifyHeader(data.peek())) {
#ifdef DEBUG_UPDATER
        printError(DEBUG_UPDATER);
#endif
        _reset();
        return 0;
    }
    esp8266::polledTimeout::oneShotMs timeOut(streamTimeout);
    if (_progress_callback) {
        _progress_callback(0, _size);
    }
    if(_ledPin != -1) {
        pinMode(_ledPin, OUTPUT);
    }

    while(remaining()) {
        if(_ledPin != -1) {
            digitalWrite(_ledPin, _ledOn); // Switch LED on
        }
        size_t bytesToRead = _bufferSize - _bufferLen;
        if(bytesToRead > remaining()) {
            bytesToRead = remaining();
        }
        toRead = data.readBytes(_buffer + _bufferLen,  bytesToRead);
        if(toRead == 0) { //Timeout
          if (timeOut) {
            _currentAddress = (_startAddress + _size);
            _setError(UPDATE_ERROR_STREAM);
            _reset();
            return written;
          }
          delay(100);
        } else {
          timeOut.reset();
        }
        if(_ledPin != -1) {
            digitalWrite(_ledPin, !_ledOn); // Switch LED off
        }
        _bufferLen += toRead;
        if((_bufferLen == remaining() || _bufferLen == _bufferSize) && !_writeBuffer())
            return written;
        written += toRead;
        if(_progress_callback) {
            _progress_callback(progress(), _size);
        }
        yield();
    }
    if(_progress_callback) {
        _progress_callback(progress(), _size);
    }
    return written;
}

void UpdaterClass::_setError(int error){
  _error = error;
#ifdef DEBUG_UPDATER
  printError(DEBUG_UPDATER);
#endif
  _reset(); // Any error condition invalidates the entire update, so clear partial status
}

void UpdaterClass::printError(Print &out){
  out.printf_P(PSTR("ERROR[%u]: "), _error);
  if(_error == UPDATE_ERROR_OK){
    out.println(F("No Error"));
  } else if(_error == UPDATE_ERROR_WRITE){
    out.println(F("Flash Write Failed"));
  } else if(_error == UPDATE_ERROR_ERASE){
    out.println(F("Flash Erase Failed"));
  } else if(_error == UPDATE_ERROR_READ){
    out.println(F("Flash Read Failed"));
  } else if(_error == UPDATE_ERROR_SPACE){
    out.println(F("Not Enough Space"));
  } else if(_error == UPDATE_ERROR_SIZE){
    out.println(F("Bad Size Given"));
  } else if(_error == UPDATE_ERROR_STREAM){
    out.println(F("Stream Read Timeout"));
  } else if(_error == UPDATE_ERROR_NO_DATA){
    out.println(F("No data supplied"));
  } else if(_error == UPDATE_ERROR_MD5){
    out.printf_P(PSTR("MD5 Failed: expected:%s, calculated:%s\n"), _target_md5.c_str(), _md5.toString().c_str());
  } else if(_error == UPDATE_ERROR_SIGN){
    out.println(F("Signature verification failed"));
  } else if(_error == UPDATE_ERROR_FLASH_CONFIG){
    out.printf_P(PSTR("Flash config wrong real: %d IDE: %d\n"), ESP.getFlashChipRealSize(), ESP.getFlashChipSize());
  } else if(_error == UPDATE_ERROR_NEW_FLASH_CONFIG){
    out.printf_P(PSTR("new Flash config wrong real: %d\n"), ESP.getFlashChipRealSize());
  } else if(_error == UPDATE_ERROR_MAGIC_BYTE){
    out.println(F("Magic byte is wrong, not 0xE9"));
  } else if (_error == UPDATE_ERROR_BOOTSTRAP){
    out.println(F("Invalid bootstrapping state, reset ESP8266 before updating"));
  } else {
    out.println(F("UNKNOWN"));
  }
}

UpdaterClass Update;
