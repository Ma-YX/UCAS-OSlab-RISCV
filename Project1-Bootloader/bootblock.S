#include <asm/sbidef.h>
#include <asm/sbiasm.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x510001f8 //first kernel in 1f8, second in 1fc

//new bootblock address
.equ bootloader, 0x51000000
//bootloader code except copy part
.equ bootloader_start, 0x51000022

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50200000 		//S_core: 0x50201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50200000	//S_core: 0x50201000


.text
.global main

main:
	fence
	//relocate bootloader
	la	a0, bootloader
	li	a1, 1
	li	a2, 0
	SBI_CALL SBI_SD_READ
	fence.i
	la	t0, bootloader_start
	jr	t0

	// fence on all memory and I/O
	fence
	//refresh i cache
	fence.i

	// print a \n to avoid system output error...
	li	a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR

	// 1) task1 call BIOS print string "It's bootblock!"
	la	a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR

	//task4 choose kernel
	li	t0, -1
L0:
	SBI_CALL SBI_CONSOLE_GETCHAR
	fence.i
	//SBI_CALL SBI_CONSOLE_PUTCHAR
	beq a0, t0, L0

	lui	t0, %hi(os_size_loc)
	addi	t0, t0, %lo(os_size_loc)
	li	a1, 49
	beq a0, a1, L1

	//load kernel_2
	lh	a1, 4(t0)
	lh	a2, (t0)
	addi	a2, a2, 1
	j	L2

L1:
	//load kernel_1
	lh	a1, (t0)
	li	a2, 1

L2:
	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
	la		a0, kernel					//mem_address
	//la 		a3, os_size_loc
	//lh		a1, (a3)					//num_of_blocks
	//li		a2, 1						//block_id
	SBI_CALL SBI_SD_READ
	fence.i
	la		t0, kernel_main
	jr		t0

	

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\nType 1 to choose kernel_1, else kernel_2\n\r"

