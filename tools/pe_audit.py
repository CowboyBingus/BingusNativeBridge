"""Audit the built plugin: what a reviewer should be able to rely on.

Checks: x64 DLL, no entry point (nothing runs at load), no imports, exactly
one export (get_plugin_api), no TLS callbacks, ASLR/NX/high-entropy flags,
version resource fields, and no local paths embedded in the file.

usage: python pe_audit.py <plugin.dll> --version X.Y.Z
"""
from __future__ import annotations

import argparse
import ctypes
from ctypes import wintypes
from pathlib import Path
import struct
import sys

PRODUCT_NAME = "Bingus Native Bridge"
FILE_NAME = "bingus_native_bridge_pluginw64_release.dll"
EXPORTS = ["get_plugin_api"]
DYNAMIC_BASE, HIGH_ENTROPY_VA, NX_COMPAT = 0x0040, 0x0020, 0x0100
EXPORT_DIR, IMPORT_DIR, TLS_DIR, DELAY_IMPORT_DIR = 0, 1, 9, 13


class Pe:
    def __init__(self, data: bytes):
        self.data = data
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        if data[pe:pe + 4] != b"PE\0\0":
            raise ValueError("not a PE file")
        self.machine, sections = struct.unpack_from("<HH", data, pe + 4)
        optional_size, self.characteristics = struct.unpack_from("<HH", data, pe + 20)
        opt = pe + 24
        self.magic = struct.unpack_from("<H", data, opt)[0]
        self.entry = struct.unpack_from("<I", data, opt + 16)[0]
        self.dll_characteristics = struct.unpack_from("<H", data, opt + 70)[0]
        count = struct.unpack_from("<I", data, opt + 108)[0]
        self.dirs = [struct.unpack_from("<II", data, opt + 112 + 8 * i) for i in range(count)]
        table = opt + optional_size
        self.sections = []
        for i in range(sections):
            at = table + 40 * i
            vsize, va, rsize, raw = struct.unpack_from("<IIII", data, at + 8)
            self.sections.append((va, max(vsize, rsize), raw))

    def offset(self, rva: int) -> int:
        for va, size, raw in self.sections:
            if va <= rva < va + size:
                return rva - va + raw
        raise ValueError(f"RVA {rva:#x} outside sections")

    def directory(self, index: int) -> tuple[int, int]:
        return self.dirs[index] if index < len(self.dirs) else (0, 0)

    def exports(self) -> list[str]:
        rva, size = self.directory(EXPORT_DIR)
        if not size:
            return []
        at = self.offset(rva)
        names = struct.unpack_from("<I", self.data, at + 24)[0]
        names_rva = struct.unpack_from("<I", self.data, at + 32)[0]
        result = []
        for i in range(names):
            name_rva = struct.unpack_from("<I", self.data, self.offset(names_rva) + 4 * i)[0]
            start = self.offset(name_rva)
            result.append(self.data[start:self.data.index(b"\0", start)].decode("ascii"))
        return result


def version_strings(path: Path) -> dict[str, str]:
    version = ctypes.WinDLL("version.dll")
    size = version.GetFileVersionInfoSizeW(str(path), None)
    if not size:
        return {}
    buffer = ctypes.create_string_buffer(size)
    if not version.GetFileVersionInfoW(str(path), 0, size, buffer):
        return {}
    result = {}
    for key in ("ProductName", "ProductVersion", "FileVersion", "OriginalFilename", "LegalCopyright"):
        pointer, length = ctypes.c_void_p(), wintypes.UINT()
        query = f"\\StringFileInfo\\040904B0\\{key}"
        if version.VerQueryValueW(buffer, query, ctypes.byref(pointer), ctypes.byref(length)) and length.value:
            result[key] = ctypes.wstring_at(pointer, length.value).rstrip("\0")
    return result


def audit(path: Path, expected_version: str) -> list[str]:
    data = path.read_bytes()
    pe = Pe(data)
    problems = []

    def require(condition: bool, message: str) -> None:
        if not condition:
            problems.append(message)

    require(pe.machine == 0x8664 and pe.magic == 0x20B, "not an x64 PE32+ image")
    require(pe.characteristics & 0x2000 != 0, "not a DLL")
    require(pe.entry == 0, f"has an entry point ({pe.entry:#x}); nothing may run at load")
    require(pe.directory(IMPORT_DIR)[1] == 0, "imports other DLLs")
    require(pe.directory(DELAY_IMPORT_DIR)[1] == 0, "has delay-load imports")
    require(pe.directory(TLS_DIR)[1] == 0, "has TLS callbacks")
    require(pe.exports() == EXPORTS, f"exports {pe.exports()}, expected {EXPORTS}")
    for flag, name in ((DYNAMIC_BASE, "ASLR"), (HIGH_ENTROPY_VA, "high-entropy ASLR"), (NX_COMPAT, "NX")):
        require(pe.dll_characteristics & flag != 0, f"{name} flag missing")
    lowered = data.lower()
    for marker in (b"c:\\users", b"\\users\\", b".pdb"):
        require(marker not in lowered, f"embeds {marker.decode()!r}; local paths must not ship")

    strings = version_strings(path)
    require(strings.get("ProductName") == PRODUCT_NAME, f"ProductName {strings.get('ProductName')!r}")
    require(strings.get("ProductVersion") == expected_version, f"ProductVersion {strings.get('ProductVersion')!r}")
    require(strings.get("FileVersion") == expected_version, f"FileVersion {strings.get('FileVersion')!r}")
    require(strings.get("OriginalFilename") == FILE_NAME, f"OriginalFilename {strings.get('OriginalFilename')!r}")
    require("BSD Zero Clause" in strings.get("LegalCopyright", ""), "LegalCopyright names no license")
    require(path.name == FILE_NAME, f"file name {path.name!r}, expected {FILE_NAME!r}")
    return problems


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("plugin", type=Path)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    problems = audit(args.plugin, args.version)
    for problem in problems:
        print(f"FAIL: {problem}")
    if problems:
        return 1
    print(f"PASS: {args.plugin.name} {args.version}: no entry point, no imports, one export, version resource set")
    return 0


if __name__ == "__main__":
    sys.exit(main())
