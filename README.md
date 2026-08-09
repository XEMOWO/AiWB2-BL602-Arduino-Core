# Ai-WB2-12F Arduino core（开发中）

Ai-Thinker Ai-WB2-12F（BL602，RISC-V + WiFi/BLE）的 Arduino 板级支持包。

## 当前状态（2026-08-08）

- ✅ **core 已能在 SDK make 下编译链接**：Blink（`pinMode`/`digitalWrite`/`delay`）→ 78KB 可烧录 `.bin`，全程零警告（`-Werror=all` 下）
- ✅ 外设已实现：GPIO（pinMode/digitalWrite/digitalRead，含上/下拉）、时间（millis/micros/delay/delayMicroseconds）、**Serial**（begin/print/println/printf/read/available/peek/flush）、PWM（analogWrite）、ADC（analogRead）、I2C（Wire）、SPI、外部中断（attachInterrupt）、EEPROM/Preferences、tone()/pulseIn()
- ✅ **硬件验证通过**：GPIO/时间/printf（2M 串口横幅）+ **Serial 双向**（115200 下 print 各进制/浮点/printf 全对，终端敲字回显成功）。PWM/ADC/I2C/SPI/中断/EEPROM 逐个外设测试中
- ✅ **WiFi**：`libraries/WiFi`（ESP32 兼容 API：WiFi.begin/status/localIP/RSSI，`WiFiConnect` 示例）
- ✅ **BLE**：`libraries/BLE`（BLE 5.0 GATT 外设：BLE.begin/send/writeReceived/connected，`BlePeripheral` 示例，GATT TX-notify/RX-write 双特征。blestack 宏是纯 C，逻辑在 `ble_native.c` C shim，BLE.h/cpp 是薄 C++ 封装）
- ✅ **原生 Arduino 构建**（真实 gcc 配方，不再委托 SDK make）：platform.txt 有真实 `recipe.*.o/combine/objcopy` + 打包在 `lib/` 的预编译 SDK 静态库 + 上传用 `tools/bflb_iot_tool.exe`。已在 WSL2 用 `tools/ide_sim_build.sh` 模拟 IDE 构建出 67.8KB 可烧录 bin（与 SDK make 产物同量级）。
- ✅ **Windows Arduino IDE 2.x 全链路实测通过**（2026-08-08）：`install_windows.bat` 装进 Arduino15 → arduino-cli compile → 烧录 → 真机串口验证全过（见下文「串口验证」）。示例走 `libraries/AiWB2/examples/`（arduino-cli 只索引库内示例，顶层 `examples/` 不显示——勿改回）。

## 目录结构

```
aiwb2-arduino/
├── cores/arduino/          # Arduino API 实现（同时也是 SDK 组件，见 bouffalo.mk）
│   ├── Arduino.h           # 主头文件
│   ├── wiring.c            # millis/micros/delay
│   ├── wiring_digital.c    # pinMode/digitalWrite/digitalRead
│   ├── wiring_analog.c     # analogRead/analogWrite（stub）
│   ├── main.cpp            # arduino_main()：setup/loop 进 FreeRTOS 任务
│   └── bouffalo.mk         # SDK 组件描述
├── variants/wb2-12f/
│   ├── pins_arduino.h      # pin 映射声明（extern）
│   └── variant.cpp         # pin 映射定义
├── boards.txt              # Arduino 板卡定义
├── platform.txt            # 编译/上传配方（草稿）
├── tools/sdk_build.sh      # sketch → bin 包装脚本（已验证）
├── package_aiwb2_index.json# Boards Manager 清单骨架
└── libraries/AiWB2/examples/   # 示例（走库结构，arduino-cli 才索引）
```

## 设计决策

1. **不重写硅片驱动，只写 Arduino 层**。core 依赖 Ai-Thinker WB2 SDK（bl_iot_sdk）的 `bl_gpio_*`/`bl_timer_*` 等封装（`components/platform/hosal/bl602_hal/`），这层是芯片厂商 SDK，无法绕开。ESP8266/ESP32 的 Arduino core 同理。
2. **同一个目录既是 Arduino core 又是 SDK 组件**。`cores/arduino/bouffalo.mk` 让 SDK make 能直接编译它；`boards.txt`/`platform.txt` 让 Arduino IDE 将来能调用它。
3. **构建走 SDK make 系统**（包装在 `tools/sdk_build.sh`）：Arduino 的逐文件编译用 `touch` 占位，combine 步骤调 SDK make 出 `.bin`。好处是不用复制 SDK 的所有编译 flag 和链接脚本；坏处是编译慢（首次全量）。后续可优化为"预编译 SDK 静态库 + 原生 Arduino 构建"。

## 怎么用

### 命令行验证（本机已验证）

```bash
# 1. 把 core 挂进 SDK 工程（参考 applications/peripherals/arduino_blink/）
cd <sdk>/applications/peripherals/arduino_blink && make
# 产物: build_out/arduino_blink.bin

# 2. 或用包装脚本
bash tools/sdk_build.sh <build_path> <project_name> <sdk_path> <sketch_dir> <core_path>
```

### 烧录

```bash
cd <sdk>/tools/flash_tool
./bflb_iot_tool-ubuntu --chipname=BL602 --port=/dev/ttyUSB0 --baudrate=921600 \
  --pt=<sdk>/tools/flash_tool/chips/bl602/partition/partition_cfg_2M.toml \
  --dts=<sdk>/tools/flash_tool/chips/bl602/device_tree/04-bl_factory_params_IoTKitA_40M-20220625.dts \
  --boot2=<sdk>/tools/flash_tool/chips/bl602/builtin_imgs/boot2_isp_bl602_v6.5.1/boot2_iap_release.bin \
  --firmware=<path-to>.bin
```

### Arduino IDE（Windows，已实测编译通过）

原生构建已就绪：platform.txt 是真实 gcc 配方，预编译 SDK 静态库在 `lib/`，
工具链 **打包进 `tools/riscv-msys`**（install 时从 SDK 拷来，见下）。

1. 打包：`zip -r aiwb2-arduino-0.1.0.zip .`（或 `python3` 打 zip，Windows 原生解压）
2. Windows 解压后双击 `install_windows.bat`
   （自动拷进 `%LOCALAPPDATA%\Arduino15\packages\aithinker\hardware\wb2\0.1.0\`，改写 `platform.txt` 的 `sdk.path=`，
   **并把 SDK 的 RISC-V 工具链拷成包内 `tools\riscv-msys`**——约 1GB，一次性）
3. **SDK 路径（二选一）**：
   - **零拷贝（推荐）**：SDK 留在 WSL2，`sdk.path` 填 `\\wsl.localhost\Ubuntu\root\wb2-12f-desktop-clock`（install 脚本会自动探测并默认填好）。Windows IDE 经 9P 共享直接读**头文件和 ld script**（纯文件读取，UNC 没问题），与 Linux 树永远同步。
   - **拷到 Windows**：`cp -r --reflink=auto` 或 rsync 整树到 D 盘，填 Windows 路径。
4. 重启 Arduino IDE 2.x → 选中板（Tools > Board > Ai-Thinker Ai-WB2-12F）→
   File > Examples > Ai-Thinker Ai-WB2-12F > AiWB2 > 02.Serial > SerialEcho
   （示例放在 `libraries/AiWB2/examples/`，arduino-cli 只从 libraries 索引示例——
   顶层 `examples/` 目录不会出现在菜单里，这是 arduino-cli 的机制，勿改成顶层目录）
5. 选板（Tools > Board > Ai-Thinker Ai-WB2-12F）、选 COM 口、Upload
   （上传用打包自带的 `tools/bflb_iot_tool.exe`，921600；板子接线 TX=GPIO16/RX=GPIO7，串口 115200 看输出）

**2026-08-08 实测结论**：`arduino-cli compile`（IDE 2.x 同款后端）已在本机跑通
SerialEcho——工具链必须 Windows 本地（gcc 从 UNC 启动不能派生子进程 cc1plus，
`CreateProcess` 失败），因此工具链打包进 `tools/riscv-msys`；`sdk.path` 走 UNC 没问题。
产物 `SerialEcho.ino.bin` 67844 字节，与 SDK/Linux 构建同量级、功能一致。

**烧录 + 串口验证（同板子实测）**：
- 烧录用 WSL2 里 SDK 的 `bflb_iot_tool-ubuntu`（板子经 `usbipd attach --wsl --busid <CH340>` 进 WSL，`/dev/ttyUSB0`），`[All Success]` + SHA 校验通过。
- **进入下载模式必须手动组合键**：按住 BOOT（GPIO8 拉低）→ 点按 RST（CHIP_EN）→ 松开 BOOT。板子没有 ESP32 那种 DTR/RTS 自动复位电路，直接烧会 `shake hand fail`。
- 复位运行：按 RST（不按 BOOT）。开机 boot2 日志在默认 2M 波特率下会花屏，属正常；`Serial.begin(115200)` 在 reset 后约 0.6s 把 console 重调，之后输出干净。
- 115200 下实测输出：`Ai-WB2-12F SerialEcho: ready.`、`print(): int=42 hex=2A bin=101 float=3.14`、`printf(): -7 0xbeef ok`，且**回显验证通过**（`hello wb2 123` 原样返回）。Serial 双向 OK。

命令行快速验证（不碰 IDE）：`bash tools/ide_sim_build.sh libraries/AiWB2/examples/02.Serial/SerialEcho/SerialEcho.ino /tmp/b` → `/tmp/b/SerialEcho.bin`。

> Boards Manager 发布（`package_aiwb2_index.json`，已填好 checksum/size）留待有 HTTP 托管后启用。

### 原生构建原理（platform.txt 关键点）

- **flags 照抄 SDK** `make_scripts_riscv/project.mk`：`-march=rv32imfc -mabi=ilp32f -Os -gdwarf`，C++ 加 `-fno-rtti -fno-exceptions -nostdlib`；宏集 `_GNU_SOURCE / ARCH_RISCV / configUSE_TICKLESS_IDLE=0 / FEATURE_WIFI_DISABLE=1 / CFG_COMPONENT_BLOG_ENABLE=0`（省略带引号的版本字符串宏——只影响日志，blog 已禁用）。
- **链接=gcc 驱动 + `--start-group` 包住全部归档**：core.a、用户库、预编译 SDK 库都要在组内（归档是懒加载，跨库符号靠组内重扫解析）。ld script `flash_rom.ld`（`CONFIG_LINK_ROM=1` 的 ROM-driver 构建）。`-Wl,--gc-sections -Wl,-static`。
- **bin = objcopy 剥掉 `.romdata/.rom/.bugkiller*`** 段（SDK 的 flash 镜像即此）。
- **`main()` 由 core 提供**（`cores/arduino/main.c` → `arduino_main()`，SDK 测试 app 不再各自定义 main）。`main.cpp` 因与 `main.c` 都产出 `main.o` 已改名 `arduino_main.cpp`。
- **`{object_files}` 必须在 combine 配方里**：sketch 的 `setup()/loop()` 在那里；丢了会 `undefined reference`（曾被 `--noinhibit-exec` 掩盖 → 已移除该 flag 让真实错误浮出）。

## 串口验证（推荐先做这一步）

烧录 + 串口一起验证 core 是否真的跑起来了。**串口接线**：SDK 日志 console 默认在
**UART0，TX=GPIO16 / RX=GPIO7，波特率 2000000（2M）**。

```bash
chmod +x tools/flash.sh
# 1. 烧录（板子接好后）
tools/flash.sh applications/peripherals/arduino_serial/build_out/arduino_serial.bin /dev/ttyUSB0
# 2. 开串口看输出（2M 波特率）
python3 tools/serial_monitor.py --port /dev/ttyUSB0 --baud 2000000
# 应循环打印：
#   Ai-WB2-12F Arduino core: setup() done, entering loop()
#   Ai-WB2-12F Arduino core: setup()/loop() running
```

**WSL2 下让 Windows 主机的板子 USB 进到 Linux**（board 插在 Windows 上时）：
```powershell
# Windows PowerShell（管理员）
usbipd list                       # 找 CH340 之类的 USB-UART，记下 BUSID
usbipd bind --busid <BUSID>       # 首次执行一次
usbipd attach --wsl --busid <BUSID>
# 回到 WSL2：ls /dev/ttyUSB0  应该出现了
```

如果不想折腾 WSL2，直接在 Windows 上验证也行：烧录用 `tools/flash_tool/bflb_iot_tool.exe`（参数同上），串口用任意终端工具（SSCOM/XCOM/Arduino IDE 串口监视器）波特率设 **2000000**。

> 如果 2M 波特率下串口花屏：部分 USB-UART 芯片对 2M 支持不稳。可以改
> `components/platform/soc/bl602/bl602/bfl_main.c:41` 的
> `HOSAL_UART_DEV_DECL(uart_stdio, 0, 16, 7, 2000000)` 为 921600（注意 GPIO16/7
> 换成 2/4 需同步改），重新编译后再测。

## 踩坑记录

- **SDK Makefile 的 `BL60X_SDK_PATH ?= $(pwd)/../../..` 是按 `applications/peripherals/<app>` 三层深度写的**。app 放两层目录会多跳一级导致找不到 project.mk。
- **工程组件目录必须有 `bouffalo.mk`**，否则 SDK make 不会发现/编译它（main.c 等不会被链接，报 `undefined reference to 'main'`）。
- **SDK 用 `-Werror=all`**：`pins_arduino.h` 里 `static const` 数组在未使用的 TU 会报 "defined but not used" → 用 `extern` 声明 + `variant.cpp` 定义。
- **`main()` 在 FreeRTOS 调度器启动后作为任务运行**（`bfl_main.c`），所以 `arduino_main()` 里可以直接 `xTaskCreate`。
- **`bl_timer_now_us()` 直接读 RISC-V mtime**，开机即跑，无需初始化。
- **SDK 的 C 头没有 `extern "C"` 保护**（bl_uart.h/bl_irq.h/bl602_uart.h 均无 `__cplusplus` 分支）。C++ 里 include 会符号 mangle，链接报 `undefined reference to 'bl_uart_data_send(unsigned char, unsigned char)'`（错误名带参数类型 = mangled）。修法：`extern "C" { #include <bl_uart.h> }`。**每个外设头都要这么包**（见 HardwareSerial.cpp）。
- **`UART0_IRQHandler` 在 ROM-driver 构建下不存在**（`CONFIG_BL602_USE_ROM_DRIVER:=1` 时 `BL602_USE_HAL_DRIVER` 未定义）。自写 ISR + `bl_irq_register(UART0_IRQn, (void*)handler)`，函数指针显式 cast 成 `void*`。
- **CLI 抢 UART0 RX**：`CONFIG_SYS_AOS_CLI_ENABLE:=1` 时 `aos_cli_init` 轮询 `/dev/ttyS0` 吃光收包 → Arduino 工程必须 `:=0`。
- **usbipd + CH340 在 WSL2 不稳**（反复 disconnect、Errno 5）；备选 `usbipd detach` 回 Windows 用 SSCOM 直连。pyserial 打开端口默认置 DTR/RTS 会复位板子 → 打开后 `ser.dtr=False; ser.rts=False`。
