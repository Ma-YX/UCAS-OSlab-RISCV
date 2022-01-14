#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>
// #define BOOT_LOADER_SIG_OFFSET 0x1fe
// #define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2)
// #define OS_SIZE_LOC 0x1fc


.equ os_size_loc, 0x502001fc

// 1. kernel address (move kernel to here ~)
.equ kernel, 0x50301000

// 2. kernel main address (jmp here to start kernel main!)
.equ kernel_main, 0x50301000

.text
.global main

main:
	// fence on all memory and I/O
	fence
	fence.i
	
    move s0,a0
	move s1,a1
	// print a \n to avoid system output error...
	li a0, 10
	SBI_CALL SBI_CONSOLE_PUTCHAR
       
    move a0, s0
	bnez a0, secondary
	// 1) task1 call BIOS print string "It's bootblock!"
	la a0, msg
	SBI_CALL SBI_CONSOLE_PUTSTR

	// 2) task2 call BIOS read kernel in SD card and jump to kernel start	
	li a5,48
	li a6,49	
L2:
	SBI_CALL SBI_CONSOLE_GETCHAR
    bne a0,a5,L2
	
	la a0, kernel
	li a2, 1
	la t0, 0x502001fc
	lh a1, (t0)
	lh a3, (t0)
	li t1, 64
	li t0, 32768
	ble a1, t1, remain

readagain:
	move a1,t1
	move t2,a0
	SBI_CALL SBI_SD_READ
	move a0,t2
	add a2, a2, t1
	add a0, a0, t0
	sub a3, a3, t1
	bgt a3, t1, readagain
	
remain:
        move a1,a3
        SBI_CALL SBI_SD_READ
        
	la t0, kernel_main
	jr t0  

// while(1) --> stop here
secondary:
	/* TODO: for slave core
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */
	csrw CSR_SIE, zero
  	csrw CSR_SIP, zero

  	la t0, kernel_main 
  	csrw CSR_STVEC, t0
	
	li t0, SIE_SSIE
  	csrw CSR_SIE  , t0
	li t0, SR_SIE  
  	csrs CSR_SSTATUS, t0 
wait_for_wakeup:
	wfi
	j wait_for_wakeup
stop:
	j stop
	nop

.data

msg: .string "It's a bootloader...Press 0 to start kernel\n\r"

