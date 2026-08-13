/*
    FSTools.cpp — runtime-harmless FSTools for the Ai-WB2-12F (BL602).

    The ESP8266 FSTools library repartitions flash at runtime: it constructs an
    alternative SPIFFS/LittleFS at an absolute ESP8266 flash address (the
    FST::layout tables in FSTools.h), copies the mounted filesystem through a
    temporary partition, then formats and re-mounts the destination. The WB2's
    flash layout and FS drivers are completely different — the ESP8266 address
    math (0x40200000-based) is meaningless here, and auto-reformatting or
    relocating partitions at runtime is not supported — so every operation is a
    no-op that reports failure without touching flash. The ESP8266 FSTools
    examples (Basic_example, custom_FS_example) compile unchanged and print the
    "Default FS not found" failure path, then idle. Hardware-impossible rule.
*/

#include "FSTools.h"
#include "LittleFS.h"
#include <Esp.h>

FSTools::FSTools()
{
}

FSTools::~FSTools()
{
    reset();
}

bool FSTools::attemptToMountFS(fs::FS & fs)
{
    (void)fs;
    // No auto-format config dance on WB2: nothing to probe, report failure.
    return false;
}

bool FSTools::mountAlternativeFS(FST::FS_t type, const FST::layout & layout, bool keepMounted)
{
    (void)type; (void)layout; (void)keepMounted;
    // Would build a SPIFFS/LittleFS at an ESP8266 flash offset; not supported.
    _mounted = false;
    _layout = nullptr;
    return false;
}

bool FSTools::mounted()
{
    return _mounted;
}

bool FSTools::moveFS(fs::FS & destinationFS)
{
    (void)destinationFS;
    // Requires _mounted source (never) and a temp-partition copy loop.
    return false;
}

void FSTools::reset()
{
    _mounted = false;
    _layout = nullptr;
}

void FSTools::fileListIterator(FS & fs, const char * dirName, FST::FileCb Cb)
{
    (void)fs; (void)dirName; (void)Cb;
    // Nothing is ever mounted on WB2 via this path, so there is nothing to
    // iterate. Kept as the API requires (sketches pass a callback).
}

uint32_t FSTools::_getStartAddr(const FST::layout & layout)
{
    return (layout.startAddr - 0x40200000);
}

uint32_t FSTools::_getSize(const FST::layout & layout)
{
    return (layout.endAddr - layout.startAddr);
}

bool FSTools::_copyFS(FS & sourceFS, FS & destFS)
{
    (void)sourceFS; (void)destFS;
    return false;
}
