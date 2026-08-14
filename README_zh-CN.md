<div align="center">

[**English**](./README.md) · 🌐 **简体中文**

</div>

<p align="center">
  <img src="assets/banner-v2.svg" width="100%" alt="Ai-WB2-12F Arduino Core — Bouffalo BL602 RISC-V SoC, 192 MHz, WiFi + BLE">
</p>

<div align="center">

[![Release](https://img.shields.io/github/v/release/XEMOWO/AiWB2-BL602-Arduino-Core?label=release&color=38bdf8&style=flat-square)](https://github.com/XEMOWO/AiWB2-BL602-Arduino-Core/releases)
[![License](https://img.shields.io/github/license/XEMOWO/AiWB2-BL602-Arduino-Core?color=22c55e&style=flat-square)](LICENSE)
[![Boards Manager](https://img.shields.io/badge/install-Boards%20Manager%20one--click-00979D?style=flat-square)](#安装)
[![Arduino IDE](https://img.shields.io/badge/Arduino-IDE%202.x-22c55e?logo=arduino&logoColor=white&style=flat-square)](https://www.arduino.cc/en/software)
[![Chip](https://img.shields.io/badge/Chip-Bouffalo%20BL602-7c4dff?style=flat-square)](https://en.bouffalolab.com/)
[![Architecture](https://img.shields.io/badge/Arch-RISC--V%20RV32IMFC-38bdf8?style=flat-square)](#规格)
[![CPU](https://img.shields.io/badge/CPU-192%20MHz-0ea5e9?style=flat-square)](#规格)
[![Radio](https://img.shields.io/badge/Radio-WiFi%20b%2Fg%2Fn%20%2B%20BLE%205.0-7c4dff?style=flat-square)](#规格)

**为安信可 Ai-WB2-12F（博流 BL602）打造的新一代 Arduino core** —— 一颗带 WiFi 802.11 b/g/n 与 BLE 5.0 的 RISC-V SoC。

用**类 ESP32 的写法**写 Arduino 程序，在开发板管理器里**一键安装**，再用内置烧录工具通过串口上传——不需要配工具链、不需要下载 SDK、什么都不用装。

</div>

---

## ✨ 亮点

| | |
|---|---|
| 🚀 **一键安装** | 安装包**完全自包含**——RISC-V 工具链和全部 SDK 头文件都打进了 zip。Arduino IDE 下载解压即可用。 |
| 🧩 **类 ESP32 API** | 现有工程几乎零改动即可移植：`Serial`、`Wire`、`SPI`、`EEPROM`、`Preferences`、`WiFi`、`BLE` 都是你熟悉的写法。 |
| ✅ **真机验证** | `Serial`、PWM、ADC、SPI、外部中断、EEPROM 都在**真实的 Ai-WB2-12F 硬件**上跑过，不只是编译通过。 |
| 🗂️ **25 个示例** | 从 `Blink` 到 `DS18B20`、NeoPixel、IR（NEC）、舵机、ST7789 屏驱动——都是能直接用的真实代码。 |
| 🪶 **薄而可靠** | Arduino API 只做薄薄一层，底下是厂商久经考验的 `bl_iot_sdk`——不重写硅片驱动，Flash/RAM 占用更省。 |
| 📻 **一芯多能** | WiFi + BLE + 丰富的模拟/GPIO，全在 192 MHz 的 RISC-V 核上，还只是一颗低成本模组。 |

## 📊 规格

| | |
|---|---|
| **SoC** | 博流 BL602 |
| **内核** | RISC-V RV32IMFC @ 192 MHz |
| **无线** | WiFi 802.11 b/g/n · BLE 5.0 |
| **存储** | 2 MB Flash · 276 KB SRAM |
| **GPIO** | 22 脚（其中 9 个推荐给 Arduino 工程） |
| **ADC** | 5 × 12-bit 通道 |
| **串口 · I²C · SPI · PWM** | 2 × UART · 1 × I²C · 1 × SPI · 5 路硬件 PWM |

## 🛠️ 安装

### 方式一：开发板管理器一键安装（推荐）

**1. 添加附加开发板管理器网址**

**文件 → 首选项 → 附加开发板管理器网址** 填入（多个网址用英文逗号分隔）：

```
https://raw.githubusercontent.com/XEMOWO/AiWB2-BL602-Arduino-Core/v0.1.3/package_aiwb2_index.json
```

**2. 安装开发板**

**工具 → 开发板 → 开发板管理器** → 搜索 **Ai-WB2**（或 **Ai-Thinker**）→ 点 **安装**。

**3. 选择开发板并上传**

选择 **工具 → 开发板 → Ai-Thinker Ai-WB2-12F**；USB-UART 接 `GPIO16` TX /
`GPIO7` RX，点 **上传**。

> [!NOTE]
> 包已自包含（RISC-V 工具链、SDK 头文件、烧录工具全在包内，约 119 MB），
> 安装过程自动下载解压，**装完即用**，无需单独装 SDK / 工具链。
> 首次编译较慢（要编译整套 core）；若安装后看不到开发板，重启一下 IDE。

### 方式二：手动安装（离线 / 内网）

无需 GitHub：把 `aiwb2-arduino-0.1.3.zip` 发给用户，解压得到 `aiwb2-arduino/`
目录，整目录拷到：

```
Windows:  %LOCALAPPDATA%\Arduino15\packages\aithinker\hardware\wb2\0.1.3
macOS:    ~/Library/Arduino15/packages/aithinker/hardware/wb2/0.1.3
```

重启 Arduino IDE 即可。包内自带工具链，同样无需额外安装。

> [!TIP]
> 包内的 `install_windows.ps1` 是**开发者调试**用的旧式安装（会把 `sdk.path`
> 指向外部 SDK 路径），**最终用户无需运行**——开发板管理器安装更简单。

## 💡 第一个程序

```cpp
#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Hello from Ai-WB2-12F!");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  delay(500);
}
```

## 📍 引脚图

<p align="center">
  <img src="assets/pinmap.svg" width="720" alt="Ai-WB2-12F 引脚图">
</p>

有 9 个 GPIO 推荐用于 Arduino 工程。6 个与内部 Flash 复用的脚（`0、1、2、20、21、22`）请避开。`GPIO8` 是 bootstrap 脚（复位时拉低进入下载模式）。

## 📦 外设支持

| 外设 | API | 示例 | 状态 |
|---|---|---|---|
| GPIO / 数字 IO | `pinMode` · `digitalWrite` · `digitalRead` | `Blink` | ✅ 已真机验证 |
| 时间 | `millis` · `micros` · `delay` | — | ✅ 已真机验证 |
| Serial（UART0） | `HardwareSerial` | `SerialEcho` | ✅ 已真机验证 |
| Serial1（UART1） | `HardwareSerial(1)` | `Serial1Echo` | 🟢 已实现 |
| PWM | `analogWrite` | `PwmFade` | ✅ 已真机验证 |
| ADC（5 路） | `analogRead` | `AnalogReadVoltage` | ✅ 已真机验证 |
| I2C | `Wire` | `ScanAndProbe` | 🟢 已实现 |
| SPI | `SPIClass` | `SpiLoopback` · `St7789Test` | ✅ 已真机验证 |
| 外部中断 | `attachInterrupt` | `ButtonInterrupt` | ✅ 已真机验证 |
| EEPROM（Flash 持久） | `EEPROM` | `EepromCounter` | ✅ 已真机验证 |
| NVS 风格存储 | `Preferences` | `PreferencesCounter` | 🟢 已实现 |
| 音调 / 脉冲 | `tone` · `pulseIn` | `ToneMelody` | 🟢 已实现 |
| 定时器 · 看门狗 · RTC | `timer_*` · `watchdog*` · `rtc_*` | `TimerAndWatchdog` | 🟢 已实现 |
| WiFi | `WiFi`（类 ESP32） | `WiFiConnect` | 🟢 已实现 |
| BLE 5.0 GATT | `BLE` | `BlePeripheral` | 🟢 已实现 |
| 舵机 | `Servo` | `Sweep` | 🟢 已实现 |
| SoftwareSerial | `SoftwareSerial` | `SoftwareSerialEcho` | 🟢 已实现 |
| 1-Wire | `OneWire` | `DS18B20` | 🟢 已实现 |
| NeoPixel | Adafruit 兼容 | `RGBLoop` | 🟢 已实现 |
| 红外（NEC） | `IRremote` | `IRSender` · `IRreceive` | 🟢 已实现 |
| 字符串 · 数学 · 流 | `String` · `map` · `Stream` | `StringAndMath` | ✅ 已真机验证 |

**图例** —— ✅ = 编译通过**且已在真实 Ai-WB2-12F 硬件验证** · 🟢 = 已实现、已通过 IDE 模拟构建，真机测试待做。

## 📂 示例

全部 25 个示例在 `libraries/AiWB2/examples/`，会在 **文件 → 示例 → AiWB2** 中列出：

`Blink` · `SerialEcho` · `SerialBanner` · `Serial1Echo` · `PwmFade` · `AnalogReadVoltage` · `ScanAndProbe` · `SpiLoopback` · `St7789Test` · `ButtonInterrupt` · `EepromCounter` · `ToneMelody` · `TimerAndWatchdog` · `PreferencesCounter` · `StringAndMath` · `WiFiConnect` · `BlePeripheral` · `Sweep` · `SoftwareSerialEcho` · `DS18B20` · `RGBLoop` · `IRSender` · `IRreceive`

## 🔬 真机验证记录

上表每一个 ✅ 都是在真实 Ai-WB2-12F 上测出来的：

- **Serial** —— 115200 双向回显（`SerialEcho`）
- **PWM** —— `GPIO14` 平滑呼吸灯（`PwmFade`）
- **SPI** —— 驱动 **ST7789 TFT @ 8MHz**（`St7789Test`）
- **ADC** —— `GPIO12` 电位器读数 `0…4095` 线性变化（`AnalogReadVoltage`）
- **中断** —— `GPIO11` 按键 `FALLING` 沿计数（`ButtonInterrupt`）
- **EEPROM** —— 计数器的值**掉电重启后依然保留**（`EepromCounter`）

## 🧱 包里有什么

| 路径 | 内容 |
|---|---|
| `cores/arduino/` | Arduino API 实现（GPIO、Serial、PWM、ADC、定时器、中断…） |
| `variants/wb2-12f/` | 板级引脚映射（`pins_arduino.h`） |
| `libraries/` | 12 个库 + 25 个示例 |
| `lib/` | 预编译 SDK 静态库 |
| `sdk-include/` | SDK 头文件 + 链接脚本（打包时自动生成） |
| `tools/riscv-msys/` | 精简版 RISC-V 工具链（Windows，已内置） |

## 📄 开源许可

[Apache-2.0](./LICENSE) · 第三方声明见 [`THIRD_PARTY_LICENSES.md`](./THIRD_PARTY_LICENSES.md)。Arduino 层封装厂商的 `bl_iot_sdk`（Apache-2.0），没有重写任何专有硅片驱动。

---

<div align="center">

为安信可 & 博流社区用心打造 —— [English](./README.md) · [提 issue](https://github.com/XEMOWO/AiWB2-BL602-Arduino-Core/issues)

</div>
