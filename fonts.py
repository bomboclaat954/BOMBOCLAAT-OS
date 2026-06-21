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
    header = f.read(32)
    magic, version, headersize, flags, length, charsize, height, width = struct.unpack("<IIIIIIII", header)
    
    if magic != 0x864ab572:
        print("Not a valid PSF2 font")
        sys.exit(1)

    f.seek(headersize)
    
    glyph_data = f.read(length * charsize)

with open("font.h", "w") as out:
    out.write("#pragma once\n")
    out.write("#include <stdint.h>\n\n")
    out.write(f"#define FONT_WIDTH {width}\n")
    out.write(f"#define FONT_HEIGHT {height}\n")
    out.write(f"#define FONT_CHARSIZE {charsize}\n\n")
    out.write("static const uint8_t embedded_font[] = {\n")
    
    for i, byte in enumerate(glyph_data):
        out.write(f"0x{byte:02X}, ")
        if (i + 1) % 16 == 0:
            out.write("\n")
            
    out.write("\n};\n")
print(f"Parsed font: {width}x{height}. {length} characters")
