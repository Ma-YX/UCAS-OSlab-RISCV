#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>

// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc

.equ os_size_loc, 0x510001fc

//new bootblock address
.equ bootloader, 0x51000000
//bootloader code except copy part
.equ bootloader_start, 0x51000022

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50201000 		//S_core: 0x50201000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50201000	//S_core: 0x50201000

.text
.global main

main:
	fence
	//relocate bootloader
	// a0 is mhartid, a1 is the pointer of fdt
	move s0,a0
	move s1,a1
	
	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernel's initialization
	// has been finished.
 	move a0, s0
	bnez a0, secondary

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

	// 2) task2 call BIOS read kernel in SD card and jump to kernel start
	la      t0, kernel
	la 		a3, os_size_loc
	lh      t1, (a3)
	addi    t1, t1, -32
	li      t2, 1
	slli    t2, t2, 14
	la		a0, kernel					//mem_address
	li		a1, 32				     	//num_of_blocks
	li		a2, 1						//block_id
load_loop:
	SBI_CALL SBI_SD_READ
	fence.i
	add     t0, t0, t2
	mv      a0, t0
	li      a1, 32
	addi    a2, a2, 32
	blt     t1, zero, end_loop
	addi    t1, t1, -32
	blt     t1, zero, new_size
	j 		load_loop
new_size:
	add     a1, a1, t1
	j       load_loop
end_loop:


	fence.i
	la		t0, kernel_main
	jr		t0

secondary:
	/* TODO: 
	* 1. Mask all interrupts
	* 2. let stvec pointer to kernel_main
	* 3. enable software interrupt for ipi
	*/
	csrw CSR_SIE, zero
	csrw CSR_SIP, zero

	la   t0, kernel_main
	csrw CSR_STVEC, t0

	li   t0, SIE_SSIE
  	csrw CSR_SIE, t0

	li   t0, SR_SIE  //SR_SPP | SR_SPIE
  	csrs CSR_SSTATUS, t0 
	

wait_for_wakeup:
	wfi
	j wait_for_wakeup
	

// while(1) --> stop here
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...\n\r"

