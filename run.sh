nasm -f elf32 -o build/print.o lib/kernel/print.asm

nasm -f elf32 -o build/kernel.o kernel/kernel.asm

gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-stack-protector -fno-builtin -o  build/main.o kernel/main.c


gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-stack-protector -fno-builtin -o build/interrupt.o kernel/interrupt.c

gcc -m32 -I lib/kernel -I lib/ -I kernel/ -c -fno-stack-protector -fno-builtin -o build/init.o kernel/init.c

gcc -m32 -I lib/kernel -c -o build/timer.o device/timer.c

ld -m elf_i386 -Ttext 0xc0001500 -e main -o build/kernel.bin build/main.o build/init.o \
build/interrupt.o  build/print.o build/kernel.o build/timer.o


dd if=build/kernel.bin of=/usr/local/share/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc
