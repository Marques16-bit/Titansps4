#!/usr/bin/env python3
"""Gera um PE32+ x64 minimo (so cabecalhos, imports e strings) para testar o analisador no PC.
Uso: python3 tools/make_test_pe.py saida.exe"""
import struct, sys

FA, SA = 0x200, 0x1000
rdata_rva, rdata_raw = 0x2000, 0x400
rd = bytearray(0x800)

def put(rva, data):
    o = rva - rdata_rva
    rd[o:o + len(data)] = data

def name_entry(rva, hint, name):
    put(rva, struct.pack('<H', hint) + name.encode() + b'\0')
    return rva

# nomes de DLL e funcoes
dll_k32, dll_d12, dll_dxgi = 0x2400, 0x2420, 0x2430
put(dll_k32, b'KERNEL32.dll\0'); put(dll_d12, b'd3d12.dll\0'); put(dll_dxgi, b'dxgi.dll\0')
f = {n: name_entry(0x2300 + i * 0x20, i, n) for i, n in enumerate(['ExitProcess', 'GetStdHandle', 'D3D12CreateDevice', 'CreateDXGIFactory1'])}
# ILTs (8 bytes por entrada; bit 63 = ordinal)
put(0x2200, struct.pack('<QQQQ', f['ExitProcess'], f['GetStdHandle'], (1 << 63) | 5, 0))
put(0x2240, struct.pack('<QQ', f['D3D12CreateDevice'], 0))
put(0x2260, struct.pack('<QQ', f['CreateDXGIFactory1'], 0))
# descritores de import (20 bytes): OFT, time, fwd, name, FT
put(0x2000, struct.pack('<IIIII', 0x2200, 0, 0, dll_k32, 0x2200) + struct.pack('<IIIII', 0x2240, 0, 0, dll_d12, 0x2240) + b'\0' * 20)
# delay import (32 bytes): attrs, name, hmod, iat, int, bound, unload, time
put(0x2100, struct.pack('<IIIIIIII', 1, dll_dxgi, 0, 0x2260, 0x2260, 0, 0, 0) + b'\0' * 32)
put(0x2500, b'vulkan-1.dll\0Oodle oo2core_9_win64.dll\0steam_api64.dll\0')

text = bytearray(0x200); text[0] = 0xC3
data = bytearray(0x200)
dos = bytearray(64); dos[0:2] = b'MZ'; struct.pack_into('<I', dos, 0x3C, 0x80)
opt = bytearray(240)
struct.pack_into('<HBBIII', opt, 0, 0x20B, 14, 0, 0x200, 0x800, 0)
struct.pack_into('<II', opt, 16, 0x1000, 0x1000)                      # entry, base of code
struct.pack_into('<Q', opt, 24, 0x140000000)                          # image base
struct.pack_into('<II', opt, 32, SA, FA)
struct.pack_into('<HHHHHH', opt, 40, 6, 0, 0, 0, 6, 0)
struct.pack_into('<IIII', opt, 56, 0x4000, 0x200, 0, 0)               # size of image, headers, checksum, (subsystem below)
struct.pack_into('<H', opt, 68, 2)                                    # Windows GUI
struct.pack_into('<I', opt, 108, 16)
struct.pack_into('<II', opt, 112 + 1 * 8, 0x2000, 60)                 # import dir
struct.pack_into('<II', opt, 112 + 13 * 8, 0x2100, 64)                # delay import dir
fh = struct.pack('<HHIIIHH', 0x8664, 3, 0, 0, 0, 240, 0x22)
def sec(name, vsize, va, rawsize, raw, flags):
    return name.ljust(8, b'\0') + struct.pack('<IIIIIIHHI', vsize, va, rawsize, raw, 0, 0, 0, 0, flags)
secs = sec(b'.text', 0x200, 0x1000, 0x200, 0x200, 0x60000020) + sec(b'.rdata', 0x800, 0x2000, 0x800, 0x400, 0x40000040) + sec(b'.data', 0x200, 0x3000, 0x200, 0xC00, 0xC0000040)
hdr = bytearray(0x200)
hdr[0:64] = dos; hdr[0x80:0x84] = b'PE\0\0'; hdr[0x84:0x84 + 20] = fh; hdr[0x98:0x98 + 240] = opt; hdr[0x188:0x188 + len(secs)] = secs
open(sys.argv[1], 'wb').write(bytes(hdr) + bytes(text) + bytes(rd) + bytes(data))
print('ok', sys.argv[1])
