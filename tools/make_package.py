#!/usr/bin/env python3
"""Rebuild aiwb2-arduino-<version>.zip — a SELF-CONTAINED Boards Manager package.

Everything the compiler needs ships inside the zip, so an end user can install
it through Arduino IDE's Boards Manager (Additional Boards Manager URL ->
package_aiwb2_index.json) with zero extra setup:

    sdk-include/   SDK C headers (2155 .h, tree structure preserved) +
                   the linker script flash_rom.ld. Generated from the bl_iot_sdk
                   checkout at package time. platform.txt points
                   sdk.path={runtime.platform.path}/sdk-include here.
    tools/riscv-msys   Windows RISC-V gcc 10.2.0, trimmed to the files an
                   rv32imfc/ilp32f compile actually needs (no gdb/python/docs,
                   one multilib instead of the ~40 shipped). Generated from the
                   SDK's toolchain/riscv/MSYS. platform.txt points
                   toolchain.path={runtime.platform.path}/tools/riscv-msys.
    lib/           prebuilt SDK static archives (checked into the repo).

The package is assembled from an explicit whitelist (not a recursive copy of
the repo) so generated dirs and the previous zip can never leak in.

Run from the repo root:
    python3 tools/make_package.py [sdk_root]
    sdk_root defaults to /root/wb2-12f-desktop-clock.
"""
import hashlib
import json
import os
import shutil
import sys
import tempfile
import zipfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = "0.1.3"
OUT = os.path.join(ROOT, f"aiwb2-arduino-{VERSION}.zip")
INDEX = os.path.join(ROOT, "package_aiwb2_index.json")
SDK = sys.argv[1] if len(sys.argv) > 1 else "/root/wb2-12f-desktop-clock"
TC = os.path.join(SDK, "toolchain/riscv/MSYS")
TCV = "riscv64-unknown-elf/10.2.0"

# ---------------------------------------------------------------------------
# 1. Repo files that go into the package (explicit whitelist).
#    tools/ = flash/upload assets (bflb_iot_tool.exe, partition cfg, boot2,
#    dts, chips/, utils/). tools/riscv-msys is generated below, not copied.
# ---------------------------------------------------------------------------
TOP_FILES = [
    "boards.txt",
    "platform.txt",
    "LICENSE",
    "README.md",
    "README_zh-CN.md",
    "THIRD_PARTY_LICENSES.md",
]
TOP_DIRS = ["cores", "libraries", "lib", "variants"]
TOOLS_KEEP = [
    "bflb_iot_tool.exe",
    "partition_cfg_2M.toml",
    "boot2_iap_release.bin",
    "04-bl_factory_params_IoTKitA_40M-20220625.dts",
    "flash.sh",
    "chips",
    "utils",
]

# ---------------------------------------------------------------------------
# 2. Minimal MSYS toolchain. Verified by compiling Blink with the Linux twin
#    (same 10.2.0 build): only these files are touched by an rv32imfc compile.
# ---------------------------------------------------------------------------
TC_BIN = [
    "riscv64-unknown-elf-gcc.exe", "riscv64-unknown-elf-g++.exe",
    "riscv64-unknown-elf-gcc-10.2.0.exe",
    "riscv64-unknown-elf-as.exe", "riscv64-unknown-elf-ld.exe",
    "riscv64-unknown-elf-ar.exe", "riscv64-unknown-elf-objcopy.exe",
    "riscv64-unknown-elf-size.exe",
    # shared libs (gcc driver + cc1/cc1plus pull libwinpthread; keep the rest
    # for margin — they total a few MB)
    "libexpat-1.dll", "libgcc_s_seh-1.dll", "libssp-0.dll",
    "libstdc++-6.dll", "libwinpthread-1.dll",
]
# lto1.exe / lto-wrapper.exe are deliberately excluded: the recipes never pass
# -flto, and dropping them saves ~22 MB — the difference between a Gitee-free
# release (100 MB single-file cap) and one that fails to upload. The linker's
# default -fuse-linker-plugin still works (liblto_plugin stays).
TC_LIBEXEC = [
    "cc1.exe", "cc1plus.exe", "collect2.exe",
    "liblto_plugin-0.dll", "liblto_plugin.dll.a",
    "liblto_plugin.la",
]
TC_LIB_GCC = ["crtbegin.o", "crtend.o", "crti.o", "crtn.o"]
TC_LIB_GCC_MULTILIB = ["rv32imfc"]
TC_SYSROOT_LIB = [
    "crt0.o", "libc.a", "libm.a", "libg.a", "libgloss.a",
    "libc_nano.a", "libm_nano.a",
]
TC_SYSROOT_LIB_MULTILIB = ["rv32imfc"]
# include/bin are small and needed whole; lib must NOT be copied whole (its
# ~40 multilib variants would add ~1 GB), so it's handled file-by-file below.
TC_SYSROOT_DIRS = ["include", "bin"]


def _cp(src, dst):
    if not os.path.exists(src):
        print(f"  !! MISSING {src}")
        return
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.isdir(src):
        shutil.copytree(src, dst, dirs_exist_ok=True)
    else:
        shutil.copy2(src, dst)


def make_riscv_msys(dst):
    """tools/riscv-msys — trimmed Windows toolchain (~350 MB raw, ~100 MB zip)."""
    print("  tools/riscv-msys (minimal MSYS toolchain)")
    os.makedirs(dst)
    b = os.path.join(dst, "bin")
    le = os.path.join(dst, "libexec/gcc", TCV)
    lg = os.path.join(dst, "lib/gcc", TCV)
    sr = os.path.join(dst, "riscv64-unknown-elf")
    for f in TC_BIN:
        _cp(os.path.join(TC, "bin", f), os.path.join(b, f))
    for f in TC_LIBEXEC:
        _cp(os.path.join(TC, "libexec/gcc", TCV, f), os.path.join(le, f))
    for f in TC_LIB_GCC:
        _cp(os.path.join(TC, "lib/gcc", TCV, f), os.path.join(lg, f))
    for f in ["include", "include-fixed"]:
        _cp(os.path.join(TC, "lib/gcc", TCV, f), os.path.join(lg, f))
    for f in TC_LIB_GCC_MULTILIB:
        _cp(os.path.join(TC, "lib/gcc", TCV, f), os.path.join(lg, f))
    for d in TC_SYSROOT_DIRS:
        _cp(os.path.join(TC, "riscv64-unknown-elf", d), os.path.join(sr, d))
    # lib/ — the rv32imfc multilib tree plus the fixed top-level archives and
    # ldscripts (default ld emulation scripts; kept for safety even though the
    # link always passes -T flash_rom.ld).
    for f in TC_SYSROOT_LIB:
        _cp(os.path.join(TC, "riscv64-unknown-elf", "lib", f),
            os.path.join(sr, "lib", f))
    for f in TC_SYSROOT_LIB_MULTILIB:
        _cp(os.path.join(TC, "riscv64-unknown-elf", "lib", f),
            os.path.join(sr, "lib", f))
    _cp(os.path.join(TC, "riscv64-unknown-elf", "lib", "ldscripts"),
        os.path.join(sr, "lib", "ldscripts"))


def make_sdk_include(dst):
    """sdk-include/ — every .h under components/, kept under a components/
    wrapper so platform.txt's existing -I{sdk.path}/components/... (and the
    -L.../evb/ld -T flash_rom.ld link script) resolve unchanged. Tree is kept
    so #include "..." lookups and the linker's relative paths still work."""
    print("  sdk-include (SDK headers + flash_rom.ld)")
    comp = os.path.join(SDK, "components")
    n = 0
    for dirpath, dirnames, filenames in os.walk(comp):
        for fn in filenames:
            if not fn.endswith(".h"):
                continue
            src = os.path.join(dirpath, fn)
            rel = os.path.relpath(src, comp)
            _cp(src, os.path.join(dst, "components", rel))
            n += 1
    print(f"    {n} headers")
    ld_src = os.path.join(comp, "platform/soc/bl602/bl602/evb/ld/flash_rom.ld")
    ld_dst = os.path.join(dst, "components/platform/soc/bl602/bl602/evb/ld/flash_rom.ld")
    _cp(ld_src, ld_dst)


# ---------------------------------------------------------------------------
# 3. Assemble + zip.
# ---------------------------------------------------------------------------
def main():
    staging = tempfile.mkdtemp(prefix="aiwb2-pkg-")
    try:
        print(f"assembling from {ROOT}")
        print(f"SDK = {SDK}")

        for f in TOP_FILES:
            _cp(os.path.join(ROOT, f), os.path.join(staging, f))
        for d in TOP_DIRS:
            _cp(os.path.join(ROOT, d), os.path.join(staging, d))
        for t in TOOLS_KEEP:
            _cp(os.path.join(ROOT, "tools", t), os.path.join(staging, "tools", t))

        make_riscv_msys(os.path.join(staging, "tools/riscv-msys"))
        make_sdk_include(os.path.join(staging, "sdk-include"))

        print(f"writing {OUT}")
        tmp = OUT + ".tmp"
        if os.path.exists(tmp):
            os.remove(tmp)
        # level 9: the toolchain .exe/.a compress noticeably harder, and every
        # MB counts against Gitee's 100 MB single-file release cap.
        with zipfile.ZipFile(tmp, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
            for dirpath, dirnames, filenames in os.walk(staging):
                dirnames.sort()
                for fn in sorted(filenames):
                    full = os.path.join(dirpath, fn)
                    # Boards Manager requires a SINGLE top-level directory (the
                    # platform root); without it, install fails with
                    # "no unique root dir in archive". Wrap everything under
                    # aiwb2-arduino/.
                    arc = os.path.join("aiwb2-arduino",
                                       os.path.relpath(full, staging)).replace(os.sep, "/")
                    zf.write(full, arc)
        os.replace(tmp, OUT)

        with open(OUT, "rb") as f:
            sha = hashlib.sha256(f.read()).hexdigest()
        size = os.path.getsize(OUT)

        with open(INDEX, "r", encoding="utf-8") as f:
            idx = json.load(f)
        platform = idx["packages"][0]["platforms"][0]
        platform["checksum"] = "SHA-256:" + sha
        platform["size"] = str(size)
        with open(INDEX, "w", encoding="utf-8") as f:
            json.dump(idx, f, indent=2, ensure_ascii=False)
            f.write("\n")

        print(f"wrote {OUT}")
        print(f"  size     = {size}")
        print(f"  sha-256  = {sha}")
        print(f"updated {INDEX}")
    finally:
        shutil.rmtree(staging, ignore_errors=True)


if __name__ == "__main__":
    main()
