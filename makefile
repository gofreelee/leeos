BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc -m32
LD = ld
LIB =  -I lib/kernel -I lib/ -I kernel/
ASFLAGS = -f elf32
CFLAGS =  -Wall $(LIB) -fno-builtin -c -W -Wstrict-prototypes \
		 -Wmissing-prototypes -fno-stack-protector
LDFLAGS = -m elf_i386 -Ttext $(ENTRY_POINT) -e main -o $(BUILD_DIR)/kernel.bin

OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
$(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o $(BUILD_DIR)/debug.o $(BUILD_DIR)/memory.o $(BUILD_DIR)/string.o \
$(BUILD_DIR)/bitmap.o

$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h lib/stdint.h kernel/interrupt.h device/timer.h kernel/debug.h
	$(CC) $(CFLAGS) -o $@ $< 

$(BUILD_DIR)/init.o: kernel/init.c kernel/init.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c kernel/interrupt.h lib/stdint.h kernel/global.h kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/stdint.h kernel/io.h lib/kernel/print.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h lib/kernel/print.h lib/stdint.h kernel/interrupt.h
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/memory.o: kernel/memory.c 
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/string.o: lib/string.c
	$(CC) $(CFLAGS) -o $@ $<

$(BUILD_DIR)/bitmap.o: lib/bitmap.c
	$(CC) $(CFLAGS) -o $@ $<


#下面编译汇编#
$(BUILD_DIR)/kernel.o: kernel/kernel.asm
	$(AS) $(ASFLAGS) -o $@ $<
$(BUILD_DIR)/print.o: lib/kernel/print.asm
	$(AS) $(ASFLAGS) -o $@ $<

#链接
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

.PHONY: hd clean all

hd:
	dd if=$(BUILD_DIR)/kernel.bin of=/usr/local/share/bochs/hd60M.img bs=512 count=200 seek=9 conv=notrunc

clean:
	cd $(BUILD_DIR) && rm -f *.o

build: $(BUILD_DIR)/kernel.bin
		   
all: build hd clean
 



