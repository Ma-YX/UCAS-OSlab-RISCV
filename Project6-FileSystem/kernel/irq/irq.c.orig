#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <os/stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <csr.h>
#include <pgtable.h>
#include <os/mm.h>

handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;
int block_num = 200;

uintptr_t swap_page(pcb_t *pcb);
int match_vaddr(pcb_t *pcb, uintptr_t vaddr);
int locate_page(pcb_t *pcb, uintptr_t vaddr);


void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    //prints("reset_irq_timer\n");
    screen_reflush();
    //sbi_console_putstr("reset_irq_timer screen_reflush\n");
    time_checking();
    //sbi_console_putstr("reset_irq_timer time_checking\n");
    // note: use sbi_set_timer
    sbi_set_timer(get_ticks() + TIMER_INTERVAL/100);
    //sbi_console_putstr("reset_irq_timer sbi_set_timer\n");
    //sbi_set_timer(0);
    // remember to reschedule
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    //sbi_console_putstr("\n\ninterrupt_helper\n");
    handler_t *table = (cause >> 63) ? irq_table : exc_table;
    uint64_t exc_code = cause & ~SCAUSE_IRQ_FLAG;
    table[exc_code](regs, stval, cause);
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    //prints("\n\nhandle_int\n");
    reset_irq_timer();
}

uintptr_t handle_inst_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    PTE *current_pte = alloc_page_checker(stval, (current_running->father)->pgdir, 2);
    if(current_pte != 0){
        set_attribute(current_pte, _PAGE_ACCESSED | _PAGE_DIRTY);
        flush_all();
        //local_flush_tlb_all();
    }
    else
        handle_other(regs,stval,cause);
}

uintptr_t handle_load_store_pagefault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    //sbi_console_putstr("\n\nload_store_pagefault\n");
    screen_reflush();
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    PTE *current_pte = alloc_page_checker(stval, (current_running->father)->pgdir, 2);
    
    int place = locate_page(current_running->father, (stval & 0xfffffffffffff000));
    if(place == 2){
        uintptr_t pa = swap_page(current_running->father);
        flush_all();
        //local_flush_tlb_all();
        int id = match_vaddr(current_running->father,stval & 0xfffffffffffff000);
        sbi_sd_read((unsigned int)pa, 8, ((current_running->father)->temp_page_table[id]).block);
        (current_running->father)->temp_page_table[id].in_mem = 1;
        (current_running->father)->temp_page_table[id].pa_ptr = pa;
        (current_running->father)->temp_page_table[id].va = stval & 0xfffffffffffff000;
        ptr_t* pte = alloc_page_checker(stval, (current_running->father)->pgdir, 1);
        set_attribute(pte, _PAGE_PRESENT | _PAGE_USER | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC);
        set_pfn(pte, pa >> 12);
        flush_all();
        //local_flush_tlb_all();
    }
    else if(place == 1){
        ptr_t *pte = alloc_page_checker(stval, (current_running->father)->pgdir, 1);
        set_attribute(pte,  _PAGE_ACCESSED | _PAGE_DIRTY);
        flush_all();
        //local_flush_tlb_all();
    }
    else{
        if(current_pte){
            set_attribute(current_pte,  _PAGE_ACCESSED | _PAGE_DIRTY);
        }
        else{
            uintptr_t pa = swap_page(current_running->father);
            uintptr_t kva = set_page_helper(stval, current_running->pgdir,current_running->father, pa);
            (current_running->father)->temp_page_table[(current_running->father)->temp_page_num].in_mem = 1;
            (current_running->father)->temp_page_table[(current_running->father)->temp_page_num].pa_ptr = pa;
            (current_running->father)->temp_page_table[(current_running->father)->temp_page_num].va = stval & 0xfffffffffffff000;
            (current_running->father)->temp_page_num++;
        }
        flush_all();
        //local_flush_tlb_all();
    }
}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    int i;
    for (i = 0;i < IRQC_COUNT;i++)
        irq_table[i] = &handle_int;
    for (i = 0;i < EXCC_COUNT;i++)
        exc_table[i] = &handle_other;
    exc_table[EXCC_SYSCALL] = &handle_syscall;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_load_store_pagefault;
    exc_table[EXCC_LOAD_PAGE_FAULT] = &handle_load_store_pagefault;
    exc_table[EXCC_INST_PAGE_FAULT] = &handle_inst_pagefault;
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}

int locate_page(pcb_t *pcb, uintptr_t vaddr){
    int i;
    for(i = 0; i < pcb->temp_page_num; i++){
        if(pcb->temp_page_table[i].va == vaddr){
            if(pcb->temp_page_table[i].in_mem)
                return 1;
            else
                return 2;
        }
    }
    return 0;
}

int match_vaddr(pcb_t *pcb, uintptr_t vaddr){
    int i;
    for(i = 0; i < pcb->temp_page_num; i++){
        if(pcb->temp_page_table[i].va == vaddr){
            return i;
        }
    }
}

uintptr_t swap_page(pcb_t *pcb){
    int count = 0;
    int i;
    for(i = 0; i < pcb->temp_page_num; i++){
        if(pcb->temp_page_table[i].in_mem == 1)
        count++;
    }
    //max = 4
    if(count < 4){
        for(i = 0;i < NUM_MAX_PPAGE; i++){
            int no_use = 1;
            int j;
            for(j = 0; j < pcb->temp_page_num; j++){
                if(pcb->pa_table[i] == pcb->temp_page_table[j].pa_ptr)
                    no_use = 0;
            }
            if(no_use == 1)
                return pcb->pa_table[i];
        }
    }
    else if(count == 4){
        for(i = 0; i < pcb->temp_page_num; i++){
            if(pcb->temp_page_table[i].in_mem == 1){
                sbi_sd_write((unsigned int)pcb->temp_page_table[i].pa_ptr, 8, block_num);
                pcb->temp_page_table[i].block = block_num;
                block_num += 8;
                pcb->temp_page_table[i].in_mem = 0;
                PTE *pte = alloc_page_checker(pcb->temp_page_table[i].va, pcb->pgdir, 1);
                *pte = ((*pte) >> 1) << 1;
                flush_all();
                //local_flush_tlb_all();
                return pcb->temp_page_table[i].pa_ptr;
            }
        }
    }
}