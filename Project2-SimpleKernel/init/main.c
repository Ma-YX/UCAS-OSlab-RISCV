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

#include <csr.h>

extern void ret_from_exception();
extern void __global_pointer$();

static void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point,
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
        pt_regs->regs[1] = entry_point;
        //pt_regs->regs[4] = (reg_t)&pcb[0];
        pt_regs->sepc = entry_point;
        pt_regs->sstatus = SR_SIE;
    } 
     
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

static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    //init pcb ready queue
    init_list_head(&ready_queue);
    init_list_head(&sleep_queue);
    task_info_t * task = NULL;
    process_id = 1;
    size_t i;
    size_t num_tasks = num_sched2_tasks + num_lock2_tasks + num_timer_tasks + num_fkpr_tasks;
   for(i = 0; i < num_tasks; i++, process_id++){
        if(i < num_sched2_tasks)
            task = sched2_tasks[i];
        else if(i < num_sched2_tasks + num_lock2_tasks)
            task = lock2_tasks[i - num_sched2_tasks];
        else if(i < num_sched2_tasks + num_lock2_tasks + num_timer_tasks)
            task = timer_tasks[i - num_sched2_tasks - num_lock2_tasks];
        else
            task = fkpr_tasks[i - num_sched2_tasks - num_lock2_tasks - num_timer_tasks];
 
        //init pcb stack
        pcb[i].kernel_sp = allocPage(1) + PAGE_SIZE;
        pcb[i].user_sp = allocPage(1) + PAGE_SIZE;
        pcb[i].type = task->type;
        init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, task->entry_point, &pcb[i]);
        //init pcb information
        pcb[i].pid = process_id;
        pcb[i].priority = PRIOR_1;
        pcb[i].status = TASK_READY;
        pcb[i].cursor_x = 0;
        pcb[i].cursor_y = 0;
        pcb[i].time_label = get_ticks();
        //add pcb[i] into ready queue
        add_queue(&ready_queue, &pcb[i]);
    }

    /* remember to initialize `current_running`
     * TODO:
     */
    current_running = &pid0_pcb;
     
}

static void init_syscall(void)
{
    // initialize system call table.
    int i;
    for(i = 0; i < NUM_SYSCALLS; i++){
        syscall[i] = 0;
    }
    syscall[SYSCALL_SLEEP       ] = &do_sleep;
    syscall[SYSCALL_YIELD       ] = &do_scheduler;
    syscall[SYSCALL_WRITE       ] = &screen_write;
    syscall[SYSCALL_READ        ] = &sbi_console_getchar;
    syscall[SYSCALL_CURSOR      ] = &screen_move_cursor; 
    syscall[SYSCALL_REFLUSH     ] = &screen_reflush;
    syscall[SYSCALL_MUTEX_GET   ] = &mutex_get;
    syscall[SYSCALL_MUTEX_LOCK  ] = &mutex_lock;
    syscall[SYSCALL_MUTEX_UNLOCK] = &mutex_unlock;
    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
    syscall[SYSCALL_GET_TICK    ] = &get_ticks;
    syscall[SYSCALL_FORK        ] = &fork;
    syscall[SYSCALL_PRIORITY    ] = &do_priority;
}

// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main()
{

    // init Process Control Block (-_-!)
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

    // init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");

    // TODO:
    // Setup timer interrupt and enable all interrupt
    sbi_set_timer(get_ticks() + time_base/1000);

    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        enable_interrupt();
        __asm__ __volatile__("wfi\n\r":::);
        //do_scheduler();
    };
    return 0;
}
