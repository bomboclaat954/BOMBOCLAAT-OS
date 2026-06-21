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

from PIL import Image
import sys
import os

def img_to_c(input_path, output_path=None, name=None):
    img = Image.open(input_path).convert("RGB")
    w, h = img.size
    
    if name is None:
        name = os.path.splitext(os.path.basename(input_path))[0]
        name = name.replace("-", "_").replace(" ", "_")
    
    if output_path is None:
        output_path = name + ".h"
    
    pixels = list(img.getdata())
    
    with open(output_path, "w") as f:
        f.write(f"#pragma once\n")
        f.write(f"#include <stdint.h>\n\n")
        f.write(f"#define {name.upper()}_WIDTH  {w}\n")
        f.write(f"#define {name.upper()}_HEIGHT {h}\n\n")
        f.write(f"static const uint32_t {name}[{w * h}] = {{\n    ")
        
        for i, (r, g, b) in enumerate(pixels):
            f.write(f"0x{r:02X}{g:02X}{b:02X}")
            if i < len(pixels) - 1:
                f.write(", ")
            if (i + 1) % 8 == 0:
                f.write("\n    ")
        
        f.write("\n};\n")
    
    print(f"Saved {output_path} ({w}x{h}, {w*h} px)")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} img.png [output.h] [name]")
        sys.exit(1)
    img_to_c(*sys.argv[1:])
