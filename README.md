# BOMBOCLAAT-OS

BOMBOCLAAT-OS is a really simple and dumb operating system (it doesn't even deserve to be called an OS) written in C for fun and to learn low-level programming. It uses the simplest possible monolithic kernel. <br>
I don't take this project too seriously, I do it for fun ~~and to prove that I'm smarter than my classmates~~. If I wanted this to be a serious project it would already be better than any modern OS (ofc it's a joke, I'm too broke to buy claude premium lol). <br>
Yes, AI was used here. But don't call me a vibecoder, most of it was written by me. *Let any one of you who is without sin be the first to throw a stone at me*. Remember that words? <br>
There's as little assembly as possible because this language is a nightmare and no one will ever understand how much I hate it. If you can, please avoid using it, your mental health will thank you.

# Main features

~~1. Own "filesystem" (rather a joke than a real fs)~~ discontinued because FAT32 is easier (still not done lol)
1. Interrupts (this shit broke my mind, please don't do it at home)
2. PC speaker support and music
3. Password option (yeah, even a chimp would break in)

Nothing special tbh, just another hobbyst OS.

# How to build

**IMPORTANT NOTE: Windows sucks in OS development so be smart and use Linux (fuck microslop btw).**
Install these packages: **gcc, nasm, xorriso, binutils, grub-pc-bin, mtools, qemu-system-x86**. If I forgot something, that's your problem.

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
Make the disk file: **THIS IS IMPORTANT!!**
```
$ make disk-img
```
Congratulations, you've just built a totally useless "operating system".
Now if you want to run this on QEMU:
```
$ make run
```
You can run it on VirtualBox or any other virtual machine if you want. Also it's possible to run this on a real computer but good luck with finding a dinosaur computer with an old CPU and BIOS (not UEFI). 

# Notes

1. I strongly recomend using QEMU for running this OS. Why? Because VirtualBox is a bit dumb and the speaker doesn't speak. If you use VBox you'll just hear nothing from commands that make sound.
2. Some part of the code is in C++ instead of C. To distinguish which headers are for C and wich for C++, the C ones end with **.h**, and the C++ ones end with **.hpp**.

# TODO

1. FAT32 or ext2
2. Installation to disk
3. Multitasking
4. Own programming language (smth like basic but better)

# Versioning system

1.x - major update (new command, functions or library) <br>
1.x.x - minor update (bug fix, small changes or changes that aren't visible in the OS)

# Contact

If you have anything useful to tell me, text me on Discord (bomboclaat954) or [Telegram](https://www.t.me/bomboclaat954).

# License

You can do with this code whatever you want, I don't give a shit.
