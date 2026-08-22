# BOMBOCLAAT-OS 2.x

BOMBOCLAAT-OS is a simple x86_64 operating system with own kernel written (mostly) in C. This version evolved evolved from BOMBOCLAAT-OS 1.x (see *legacy* branch) and is way more mature and advanced.

## Features

<ol>
    <li>Framebuffer support</li>
    <li>x86_64 long mode</li>
    <li>Process management and scheduler</li>
    <li>TMPFS</li>
    <li>FAT32 driver (read-only for now)</li>
    <li>`SYSCALL` instruction support</li>
    <li>PMM and VMM</li>
    <li>External initramfs</li>
    <li>Separate kernel and user space</li>
</ol>

Almost none of these works fully correct. Currently I'm trying not to get mentally insane but it's getting harder with every commit.

## Notes

1. If you followed versions 1.x, you may think that this project went backwards in development (because there's less commands), but actually it's the biggest progres that could happen. From a dumb, endless loop of stupid CLI it evolved into a real and (theoretically) usable kernel and OS. **This version is still in beta, so some things may not work properly or at all. If you found a bug, please report it to me.**
2. It's highly recommended to use BOMBOCLAAT-OS with UEFI; some things might not work on BIOS or errors may occur.

# Contributing

Writing an entire OS alone is hard, so any help would be really appreciated. If you have any suggestions or if you want to become a co-author, contact me and we'll discuss what can you do for the project. I need some people to help me with that.

# Contact

If you have any questions, you can text me on my [Telegram](https://t.me/bomboclaat954) or Discord (bomboclaat954).

# License

BOMBOCLAAT-OS is published as an open-source project under the GNU GPL v3 license.
