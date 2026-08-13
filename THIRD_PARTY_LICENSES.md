# Third-party components & licenses

The Ai-WB2-12F Arduino core ships against the Ai-Thinker **bl_iot_sdk** and the
RISC-V GNU toolchain. This page records what those components are, where they
come from, and what their licenses require.

## Provenance

The `cores/`, `variants/`, and `libraries/` sources in this package are
**original work** written for this core; they are not copied from Bouffalo Lab's
`arduino-bouffalo`. They implement the Arduino API on top of the SDK's C
libraries (hosal, bl_driver, etc.) the same way any Arduino core wraps a vendor
SDK. A few protocols (1-Wire ROM search, NEC IR timing) are standard published
algorithms re-implemented here; see below.

| Component | Origin | License | Notes |
|---|---|---|---|
| Ai-Thinker WB2 SDK (`bl_iot_sdk`) | Ai-Thinker / Bouffalo Lab | Apache License 2.0 | Prebuilt libraries linked into every sketch; `lib/LICENSE` in the SDK tree. This package references them; it does not re-license them. |
| FreeRTOS (in the SDK) | Real Time Engineers Ltd | MIT | The SDK's RTOS; used via `task.h`/`queue.h`. |
| newlib + libstdc++ (toolchain) | GNU project | BSD-ish / GPLv3 with GCC Runtime Exception | Standard C/C++ runtime from the RISC-V toolchain; dynamically linked, not shipped as source. |
| Classic Arduino 1-Wire ROM-search algorithm | Paul Stoffregen / Arduino OneWire | MIT / public-domain style | Re-implemented in `libraries/OneWire`; the algorithm itself is a published public-domain sequence. |
| Adafruit_NeoPixel API surface | Adafruit Industries | LGPL-3.0 | Only the public API names (`setPixelColor`, `NEO_GRB`, ...) are matched for drop-in compatibility; `libraries/NeoPixel` is an independent implementation. |

## License of this core

The core itself (everything in `cores/`, `variants/`, and `libraries/` unless
noted above) is provided under the **Apache License 2.0** — see `LICENSE`.

## Compliance notes

- Prebuilt SDK libraries are used as-is under Apache 2.0; the full SDK source is
  published by Ai-Thinker/Bouffalo Lab.
- `libraries/NeoPixel` intentionally re-implements (rather than bundles) the
  Adafruit library to keep this package LGPL-free.
- If you redistribute the compiled `.bin` files, no further obligations arise
  beyond the above notices.
