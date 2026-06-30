"""
 * BOMBOCLAAT-OS - simple x86_64 operating system
 * Copyright (C) 2026  Jakub Fietko <fietkojakub@proton.me>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
"""

import sys
import struct

if len(sys.argv) < 2:
    print("Usage: python3 fonts.py file.psf")
    sys.exit(1)

with open(sys.argv[1], "rb") as f:
    raw = f.read()

magic2 = struct.unpack_from("<I", raw, 0)[0]
magic1 = struct.unpack_from("<H", raw, 0)[0]

if magic2 == 0x864AB572:
    # PSF2
    _, version, headersize, flags, length, charsize, height, width = struct.unpack_from("<IIIIIIII", raw, 0)
    glyph_data = raw[headersize : headersize + length * charsize]
    print(f"Format: PSF2, {width}x{height}, {length} chars")

elif magic1 == 0x0436:
    # PSF1
    _, mode, charsize = struct.unpack_from("<HBB", raw, 0)
    width = 8
    height = charsize
    headersize = 4
    length = 512 if (mode & 0x01) else 256
    glyph_data = raw[headersize : headersize + length * charsize]
    print(f"Format: PSF1, {width}x{height}, {length} chars")

else:
    print(f"Unknown format! Magic bytes: {raw[:4].hex()}")
    sys.exit(1)

bytes_per_row = (width + 7) // 8
rows_per_char = charsize // bytes_per_row

if bytes_per_row == 1:
    row_type = "uint8_t"
elif bytes_per_row == 2:
    row_type = "uint16_t"
else:
    row_type = "uint32_t"

with open(f"{sys.argv[1]}.h", "w") as out:
    out.write("#pragma once\n")
    out.write("#include <stdint.h>\n\n")
    out.write(f"#define FONT_COLS  {width}\n")
    out.write(f"#define FONT_ROWS {height}\n")
    out.write(f"#define FONT_CHARS  {length}\n")
    out.write(f"#define FONT_BITS_ORDER 1\n\n")
    out.write(f"// {width}x{height} font, {bytes_per_row} byte(s) per row\n")
    out.write(f"static const {row_type} font_pixels[{length}][{rows_per_char}] = {{\n")

    for char_idx in range(length):
        out.write("    {")
        for row in range(rows_per_char):
            offset = char_idx * charsize + row * bytes_per_row
            row_bytes = glyph_data[offset : offset + bytes_per_row]
            value = int.from_bytes(row_bytes, byteorder='little')

            if bytes_per_row == 1:
                out.write(f"0x{value:02X}")
            elif bytes_per_row == 2:
                out.write(f"0x{value:04X}")
            else:
                out.write(f"0x{value:08X}")

            if row < rows_per_char - 1:
                out.write(", ")

        out.write(f"}},  // char {char_idx}\n")

    out.write("};\n")

print(f"Generated font.h: {row_type}[{length}][{rows_per_char}]")
