import sys


def convert_psf_to_c(input_file, output_file):
    with open(input_file, "rb") as f:
        data = f.read()

    if data[0] != 0x36 or data[1] != 0x04:
        print("It's not a PSF1 file")
        return

    mode = data[2]
    char_size = data[3]
    font_data = data[4:]

    num_chars = 512 if (mode & 0x01) else 256

    with open(output_file, "w") as f:
        f.write(f"// Font generated from {input_file}\n")
        f.write(f"// Character height: {char_size}px\n\n")
        f.write(f"unsigned char font[4096] = {{\n")

        for i in range(256):
            f.write(f"    // Character {i}\n    ")
            for j in range(char_size):
                byte = font_data[i * char_size + j]
                f.write(f"0x{byte:02X}, ")
            if char_size < 16:
                for _ in range(16 - char_size):
                    f.write("0x00, ")
            f.write("\n")
        f.write("};\n")
    print(f"Done")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 fonts.py <file.psf>")
    else:
        convert_psf_to_c(sys.argv[1], "font.h")
