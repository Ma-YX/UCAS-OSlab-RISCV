/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <os/lock.h>
#include <test.h>
#include <os/smp.h>
#include <csr.h>
#include <pgtable.h>
#include <os/elf.h>
#include <user_programs.h>
#include <os/ioremap.h>
#include <assert.h>
#include "xemacps_example.h"
#include <net.h>
#include <os/fs.h>

//#define QEMU
#define PYNQ

extern void ret_from_exception();
extern void __global_pointer$();

void cancel_tmp_map(){
    uint64_t va    = 0x50200000;
    uint64_t pgdir = 0xffffffc05e000000;
    uint64_t vpn2 = (va << 25) >> 55;
    uint64_t vpn1 = (va << 34) >> 55;
    PTE *pgdir_entry2 = (PTE *)pgdir + vpn2;
    PTE *pgdir_entry1 = get_pa(*pgdir_entry2) + vpn1;
    *pgdir_entry2 = 0;
    *((PTE *)pa2kva(pgdir_entry1)) = 0;
}

void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, int arg, void* argv,
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    /* TODO: initialization registers
     * note: sp, gp, ra, sepc, sstatus
     * gp should be __global_pointer$
     * To run the task in user mode,
     * you should set corresponding bits of sstatus(SPP, SPIE, etc.).
     */
     
     //init all main processor registers
    size_t i;
    for(i = 0; i < 32; i++){
       pt_regs->regs[i] = 0;
    }
    if (pcb->type == USER_PROCESS || pcb->type == USER_THREAD){    
        pt_regs->regs[2] = user_stack;
        pt_regs->regs[3] = (reg_t)__global_pointer$;
        pt_regs->regs[4] = (reg_t)pcb; 
        //pt_regs->regs[4] = (reg_t)&pcb[0];
        pt_regs->sepc = entry_point;
        pt_regs->sstatus = SR_SUM;
        pt_regs->scause = 0;
        pt_regs->sbadaddr = 0;
    } 
    pt_regs->regs[10] = (reg_t)arg; 
    pt_regs->regs[11] = (reg_t)argv;  
     
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
     //pt_regs->regs[2] = user_stack;                 //sp
     //pt_regs->regs[3] = (reg_t)__global_pointer$;   //gp
     pcb->kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
     switchto_context_t * st_regs = (switchto_context_t *)pcb->kernel_sp;
     if(pcb->type == USER_PROCESS || pcb->type == USER_THREAD){              //ra
        st_regs->regs[0] = (reg_t)&ret_from_exception;
     } else{
        st_regs->regs[0] = entry_point;
     }
     st_regs->regs[1] = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);               //sp
}
#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>

static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    //init pcb ready queue
    init_list_head(&ready_queue);
    init_list_head(&sleep_queue);
    init_list_head(&net_recv_queue);
    init_list_head(&net_send_queue);
    task_info_t * task = NULL;
    process_id = 1;
    size_t i;
    size_t num_tasks = 1;
   for(i = 0; i < num_tasks; i++, process_id++){
        task = NULL;
        //init pcb stack
        pcb[i].page_num = 0;
        pcb[i].pgdir = allocPage(&pcb[i]);
        clear_pgdir(pcb[i].pgdir);
        pcb[i].kernel_stack_base = allocPage(&pcb[i]) + PAGE_SIZE;   //a kernel virtual addr, has been mapped
        pcb[i].user_stack_base = USER_STACK_ADDR;                    //a user virtual addr, not mapped
        uintptr_t src_pgdir = pa2kva(PGDIR_PA);
        share_pgtable(pcb[i].pgdir, src_pgdir);
        alloc_page_helper(pcb[i].user_stack_base - 0x1000, pcb[i].pgdir, &pcb[i]);
        pcb[i].kernel_sp = pcb[i].kernel_stack_base;
        pcb[i].user_sp = pcb[i].user_stack_base;
        unsigned file_len = *(elf_files[0].file_length);
        user_entry_t entry_point = (user_entry_t)load_elf(elf_files[0].file_content, file_len, pcb[i].pgdir, &pcb[i], alloc_page_helper);
        flush_all();
        //local_flush_tlb_all();
        pcb[i].type = USER_PROCESS;
        pcb[i].father = &pcb[i];
        pcb[i].child_num = 0;
        init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, (ptr_t)entry_point, NULL, NULL, &pcb[i]);
        //init pcb information
        int j;
        for(j = 0; j < NUM_MAX_PPAGE; j++){
            pcb[i].pa_table[j] =allocPage(&pcb[i]) - KPA_OFFSET;
        }
        for(j = 0; j < NUM_MAX_VPAGE; j++){
            pcb[i].temp_page_table[j].block = 0;
            pcb[i].temp_page_table[j].in_mem = 0;
            pcb[i].temp_page_table[j].va = 0;
            pcb[i].temp_page_table[j].pa_ptr = 0;
        }
        
        pcb[i].mode = AUTO_CLEANUP_ON_EXIT;
        pcb[i].pid = process_id;
        pcb[i].priority = PRIOR_1;
        pcb[i].status = TASK_READY;
        pcb[i].cursor_x = 1;
        pcb[i].cursor_y = 1;
        pcb[i].time_label = get_ticks();
        pcb[i].lock_num = 0;
        pcb[i].mask = 3;
        pcb[i].shm_page_num = 0;
        init_list_head(&pcb[0].wait_list);
        //add pcb[i] into ready queue
        add_queue(&ready_queue, &pcb[i]);
        process_num++;
    }

    /* remember to initialize `current_running`
     * TODO:
     */
    current_running_m = &pid0_pcb_m;
    current_running_s = &pid0_pcb_s;
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
     
}

static void init_syscall(void)
{
    // initialize system call table.
    int i;
    for(i = 0; i < NUM_SYSCALLS; i++){
        syscall[i] = 0;
    }
    syscall[SYSCALL_SLEEP            ] = (long int (*)())&do_sleep;
    syscall[SYSCALL_YIELD            ] = (long int (*)())&do_scheduler;
    syscall[SYSCALL_WRITE            ] = (long int (*)())&screen_write;
    syscall[SYSCALL_READ             ] = (long int (*)())&sbi_console_getchar;
    syscall[SYSCALL_CURSOR           ] = (long int (*)())&screen_move_cursor; 
    syscall[SYSCALL_REFLUSH          ] = (long int (*)())&screen_reflush;
    syscall[SYSCALL_MUTEX_GET        ] = (long int (*)())&mutex_get;
    syscall[SYSCALL_MUTEX_LOCK       ] = (long int (*)())&mutex_lock;
    syscall[SYSCALL_MUTEX_UNLOCK     ] = (long int (*)())&mutex_unlock;
    syscall[SYSCALL_GET_TIMEBASE     ] = (long int (*)())&get_time_base;
    syscall[SYSCALL_GET_TICK         ] = (long int (*)())&get_ticks;
    syscall[SYSCALL_FORK             ] = (long int (*)())&do_fork;
    syscall[SYSCALL_PRIORITY         ] = (long int (*)())&do_priority;
    syscall[SYSCALL_SCREEN_CLEAR     ] = (long int (*)())&screen_clear;
    syscall[SYSCALL_SPAWN            ] = (long int (*)())&do_spawn;
    syscall[SYSCALL_PS               ] = (long int (*)())&do_process_show;
    syscall[SYSCALL_EXIT             ] = (long int (*)())&do_exit;
    syscall[SYSCALL_KILL             ] = (long int (*)())&do_kill;
    syscall[SYSCALL_WAITPID          ] = (long int (*)())&do_waitpid;
    syscall[SYSCALL_GETPID           ] = (long int (*)())&do_getpid;
    syscall[SYSCALL_GET_CHAR         ] = (long int (*)())&sbi_console_getchar;
    syscall[SYSCALL_SERIAL_WRITE     ] = (long int (*)())&screen_serial_write;
    syscall[SYSCALL_SEMAPHORE_INIT   ] = (long int (*)())&semaphore_init;
    syscall[SYSCALL_SEMAPHORE_UP     ] = (long int (*)())&semaphore_up;
    syscall[SYSCALL_SEMAPHORE_DOWN   ] = (long int (*)())&semaphore_down;
    syscall[SYSCALL_SEMAPHORE_DESTORY] = (long int (*)())&semaphore_destory;
    syscall[SYSCALL_BARRIER_INIT     ] = (long int (*)())&barrier_init;
    syscall[SYSCALL_BARRIER_WAIT     ] = (long int (*)())&barrier_wait;
    syscall[SYSCALL_BARRIER_DESTORY  ] = (long int (*)())&barrier_destory;
    syscall[SYSCALL_MBOX_OPEN        ] = (long int (*)())&mbox_open_k;
    syscall[SYSCALL_MBOX_CLOSE       ] = (long int (*)())&mbox_close_k;
    syscall[SYSCALL_MBOX_SEND        ] = (long int (*)())&mbox_send_k;
    syscall[SYSCALL_MBOX_RECV        ] = (long int (*)())&mbox_recv_k;
    syscall[SYSCALL_MBOX_SEND_RECV   ] = (long int (*)())&mbox_send_recv_k;
    syscall[SYSCALL_TASKSET_P        ] = (long int (*)())&do_taskset_p;
    syscall[SYSCALL_TASKSET_SPAWN    ] = (long int (*)())&do_taskset_spawn;
    syscall[SYSCALL_EXEC             ] = (long int (*)())&do_exec;
    syscall[SYSCALL_LS               ] = (long int (*)())&do_show_exec;
    syscall[SYSCALL_MTHREAD_CREATE   ] = (long int (*)())&do_mthread_create;
    syscall[SYSCALL_SHM_PAGE_GET     ] = (long int (*)())&shm_page_get;
    syscall[SYSCALL_SHM_PAGE_DT      ] = (long int (*)())&shm_page_dt;
    syscall[SYSCALL_NET_IRQ_MODE     ] = (long int (*)())&do_net_irq_mode;
    syscall[SYSCALL_NET_RECV         ] = (long int (*)())&do_net_recv;
    syscall[SYSCALL_NET_SEND         ] = (long int (*)())&do_net_send;
    syscall[SYSCALL_MKFS             ] = (long int (*)())&do_mkfs;
    syscall[SYSCALL_STATFS           ] = (long int (*)())&do_statfs;
    syscall[SYSCALL_CD               ] = (long int (*)())&do_cd;
    syscall[SYSCALL_FS_LS            ] = (long int (*)())&do_fs_ls;
    syscall[SYSCALL_MKDIR            ] = (long int (*)())&do_mkdir;
    syscall[SYSCALL_RMDIR            ] = (long int (*)())&do_rmdir;
    syscall[SYSCALL_TOUCH            ] = (long int (*)())&do_touch;
    syscall[SYSCALL_CAT              ] = (long int (*)())&do_cat;
    syscall[SYSCALL_OPEN_FILE        ] = (long int (*)())&do_fopen;
    syscall[SYSCALL_CLOSE_FILE       ] = (long int (*)())&do_fclose;
    syscall[SYSCALL_FREAD            ] = (long int (*)())&do_fread;
    syscall[SYSCALL_FWRITE           ] = (long int (*)())&do_fwrite;
    syscall[SYSCALL_LS_L             ] = (long int (*)())&do_fs_ls_l;
    syscall[SYSCALL_RMFILE           ] = (long int (*)())&do_rmfile;
    syscall[SYSCALL_LINK             ] = (long int (*)())&do_link;
    syscall[SYSCALL_LSEEK            ] = (long int (*)())&do_lseek;
}

// jump from bootloader.
 
    //net_poll_mode = 1;
    // xemacps_example_main();
// The beginning of everything >_< ~~~~~~~~~~~~~
void init_net(void){
    // init Process Control Block (-_-!)
        uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

        // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
        slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
        printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

        // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
        ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
        printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

        uint32_t plic_addr = 0;
        // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
        plic_addr = sbi_read_fdt(PLIC_ADDR);
        printk("[plic] plic: 0x%x\n\r", plic_addr);

        uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
        // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
        printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

        XPS_SYS_CTRL_BASEADDR =
            (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
        xemacps_config.BaseAddress =
        #ifdef PYNQ
            (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
        #endif
        #ifdef QEMU
            (uintptr_t)ioremap((uint64_t)ethernet_addr, 9 * NORMAL_PAGE_SIZE);
        #endif
        uintptr_t _plic_addr =
            (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000*NORMAL_PAGE_SIZE);
        // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
        // xemacps_config.BaseAddress = ethernet_addr;
        xemacps_config.DeviceId        = 0;
        xemacps_config.IsCacheCoherent = 0;

        printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
            XPS_SYS_CTRL_BASEADDR);
        printk(
            "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
            xemacps_config.BaseAddress);
        printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
        plic_init(_plic_addr, nr_irqs);
    
        long status = EmacPsInit(&EmacPsInstance);
        if (status != XST_SUCCESS) {
            printk("Error: initialize ethernet driver failed!\n\r");
            assert(0);
        }
}
int main()
{
    current_running_m = &pid0_pcb_m;
    current_running_s = &pid0_pcb_s;
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    uint64_t cpu_id;
    cpu_id = get_current_cpu_id();
    if(cpu_id == 0){
        smp_init();
        lock_kernel();
        //cancel_tmp_map();
        wakeup_other_hart();
        /*
        init_net();
        */
        init_pcb();
        printk("> [INIT] PCB initialization succeeded.\n\r");

        // read CPU frequency
        time_base = sbi_read_fdt(TIMEBASE);

        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // fdt_print(riscv_dtb);
        net_poll_mode = 1;
        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");

        if(!check_fs())
            do_mkfs();
    }
    else{
        //printk("> [INIT] SALVE CORE initialization succeeded.\n\r");
        //while(1);
        lock_kernel();
        setup_exception();
        printk("> [INIT] slave core succeeded.\n\r");
        //cancel_tmp_map();
    }
    // TODO:
    // Setup timer interrupt and enable all interrupt
    sbi_set_timer(get_ticks() + TIMER_INTERVAL/100);
    unlock_kernel();
    //if((cpu_id = get_current_cpu_id()) == 0)
    enable_interrupt();
    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        //enable_interrupt();
        //__asm__ __volatile__("wfi\n\r":::);
        //do_scheduler();
       //prints("break\n");
    };
    return 0;
}
