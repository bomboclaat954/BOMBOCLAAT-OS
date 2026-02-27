# BOMBOCLAAT-OS
BOMBOCLAAT-OS is a really simple and dumb operating system (I wouldn't even call it an OS) written in C for fun and to learn low-level programming.

# How to build
**IMPORTANT NOTE: Windows sucks in OS development so be smart and use Linux.**
Install these packages: **gcc, nasm, xorriso, binutils, grub-pc-bin, mtools, qemu-system-x86**.
Clone the repo:
```
$ git clone https://github.com/bomboclaat954/BOMBOCLAAT-OS.git
```
Go to cloned folder:
```
$ cd BOMBOCLAAT-OS
```
And build the system:
```
$ make all
```
Congratulations, you've just built totally useless "operating system".
Now if you want to run this on QEMU:
```
$ make run
```
You can run it on VirtualBox or any other virtual machine if you want. Also it's possible to run this on a real computer but good luck with finding a dinosaur computer with an old Intel and BIOS (not UEFI). 

# License
You can do with these files whatever you want, I don't give a shit. 
