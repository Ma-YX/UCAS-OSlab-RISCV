CC = riscv64-unknown-linux-gnu-gcc
HOST_CC = gcc
# CFLAGS = -O2  -fno-pic -fno-builtin -nostdinc -N -o bootblock bootblock.s -nostdlib -e main -Wl,-m -T riscv.lds
CFLAGS = -O2  -fno-builtin -nostdlib -T riscv.lds -Iinclude -Wall -mcmodel=medany
DISK = /dev/sdb

BOOTLOADER_ENTRYPOINT = 0x51000000
KERNEL1_ENTRYPOINT = 0x50200000
KERNEL2_ENTRYPOINT = 0x50200000

all: createimage image

bootblock: bootblock.S riscv.lds
	${CC} ${CFLAGS} -o bootblock bootblock.S -e main -Ttext=${BOOTLOADER_ENTRYPOINT}

kernel_1: kernel_1.c head.S riscv.lds
	${CC} ${CFLAGS} -o kernel_1 kernel_1.c head.S -Ttext=${KERNEL1_ENTRYPOINT}

kernel_2: kernel_2.c head.S riscv.lds
	${CC} ${CFLAGS} -o kernel_2 kernel_2.c head.S -Ttext=${KERNEL2_ENTRYPOINT}

createimage: createimage.c
	${HOST_CC} createimage.c -o createimage -ggdb -Wall

image: bootblock kernel_1 kernel_large_50200000 createimage
	./createimage --extended bootblock kernel_1 kernel_large_50200000

clean:
	rm -rf bootblock image kernel kernel_1 kernel_2 *.o createimage

floppy:
	sudo fdisk -l ${DISK}
	sudo dd if=image of=${DISK}2 conv=notrunc
