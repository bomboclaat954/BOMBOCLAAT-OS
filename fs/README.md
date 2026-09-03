# fs

Here the implementations of file systems are stored.

# fs/devfs

A file system made especially for devices that can communicate with the kernel in any way (so basically any device).

# fs/fat32

An unfinished and not working (for now) implementation of the old and reliable FAT32.

# fs/tmpfs

Sometimes error-causing fs that lives entirely in RAM, and everything that it stores is erased once the power goes down. It's used mostly in initramfs.

# fs/vfs

That's the most genius idea in the IT I've ever heard about. Instead of calling fs-specific functions you just call the VFS and don't think what fs is it. My implementation is a bit sloppy and basically not perfect, but for now it just works.
