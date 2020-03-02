nasm -f elf32 -o build/print.o lib/kernel/print.asm
gcc -m32 -I lib/kernel/ -c -o  build/main.o kernel/main.c
ld -m elf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/print.o
dd if=build/kernel.bin of=/usr/local/share/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc