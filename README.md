# BOMBOCLAAT-OS 2.x

BOMBOCLAAT-OS is a simple x86_64 operating system with own kernel written (mostly) in C. This version evolved evolved from BOMBOCLAAT-OS 1.x (see *legacy* branch) and is way more mature and advanced.

## Message from the author

Do you know the feeling when you add something new to your code and 10 other things break down? Well, I feel it every time I try to improve this piece of shit. Every time I try to add or fix anything by myself I end up asking Claude why it doesn't work and spending hours or even days to fix 10 other things. And that's why I'm taking a break from this project for at least 2 weeks (since 1.08.2026), maybe more. But don't worry, I'm not going to abandon the biggest project of my life, I just need to rest a bit. I promise I'll fix all of the errors and bugs in this code and (maybe) by the end of this year you'll see BOMBOCLAAT-OS v2.0 officially released (but I can't guarantee that). For any questions hit me up on Telegram or Discord (see **Contact** below).

## Features

<ol>
    <li>Framebuffer support</li>
    <li>x86_64 long mode</li>
    <li>Process management and scheduler (BETA)</li>
    <li>TMPFS</li>
    <li>FAT32 driver (read-only for now)</li>
    <li>Syscalls (int 0x80)</li>
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
