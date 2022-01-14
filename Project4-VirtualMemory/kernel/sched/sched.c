#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <os/string.h>
#include <os/smp.h>
#include <pgtable.h>
#include <user_programs.h>
#include <os/elf.h>

#define RR
//#define PRIORITY

#define NUM_PROGRAM 10

ptr_t stack_table[512];
short stack_index = 0;
pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack_m = INIT_KERNEL_STACK_M + PAGE_SIZE;
pcb_t pid0_pcb_m = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_m,
    .user_sp = (ptr_t)pid0_stack_m,
    .preempt_count = 0,
    .pgdir =0xffffffc05e000000,
    .father = &pid0_pcb_m,
    .page_num = 0
};
const ptr_t pid0_stack_s = INIT_KERNEL_STACK_S + PAGE_SIZE;
pcb_t pid0_pcb_s = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_s,
    .user_sp = (ptr_t)pid0_stack_s,
    .preempt_count = 0,
    .pgdir =0xffffffc05e000000,
    .father = &pid0_pcb_s,
    .page_num = 0
};

LIST_HEAD(ready_queue);
void add_queue(list_head* queue, pcb_t* item);
pcb_t *delete_queue(list_head* queue);

/* current running task PCB */
pcb_t * volatile current_running;
pcb_t * volatile current_running_m;
pcb_t * volatile current_running_s;

/* global process id */
pid_t process_id = 1;

int process_num = 0;

extern void ret_from_exception();


uint32_t rank(pcb_t *pcb){
    uint32_t ticket = pcb->priority * 5 + ((get_ticks() - pcb->time_label) / TIMER_INTERVAL);
    return ticket;
}

pcb_t *prior_search_next(list_head *queue){
    pcb_t *next, *tmp;
    list_node_t *node = queue->next;
    next = list_entry(node, pcb_t, list);
    uint32_t ticket;
    uint32_t max_ticket = 0;
    while(node != queue){
        tmp = list_entry(node, pcb_t, list);
        if((ticket = rank(tmp)) > max_ticket){
            max_ticket = ticket;
            next = tmp;
        }
        node = node->next;
    }   
    return next; 
}




void do_scheduler(void)
{
    //time_checking();
    // TODO schedule
    // Modify the current_running pointer.
    //sbi_console_putstr("\ndo_scheduler\n");
    uint64_t cpu_id = get_current_cpu_id();
    current_running = (cpu_id == 0) ? current_running_m : current_running_s;
    pcb_t *next_running, *last_running;
    next_running = last_running = current_running;
    //if(cpu_id == 1) switch_to(last_running, next_running);
    if(!list_empty(&ready_queue)){
        #ifdef RR
        next_running = delete_queue(&ready_queue);
        #endif
        #ifdef PRIORITY
        pcb_t *next_running = prior_search_next(&ready_queue);
        list_del(&next_running->list);
        next_running->time_label = get_ticks();
        #endif
        if(next_running->mask != 3 && next_running->mask != cpu_id + 1){
            add_queue(&ready_queue, next_running);
            next_running = (cpu_id == 0) ? &pid0_pcb_m : &pid0_pcb_s;
        }
        else{
            next_running->status = TASK_RUNNING;
        }
        last_running = current_running;
        if(current_running->status == TASK_RUNNING && current_running->pid != 0){
            current_running->status = TASK_READY;
            add_queue(&ready_queue, current_running);
        }
        current_running = next_running;
    }
    
    else{
        next_running = (cpu_id == 0) ? &pid0_pcb_m : &pid0_pcb_s;
        last_running = current_running;
        if(current_running != &pid0_pcb_m && current_running != &pid0_pcb_s){
            current_running->status = TASK_READY;
            add_queue(&ready_queue, current_running);
            current_running = next_running;
        }
    }

    if(cpu_id == 0){
        current_running_m = current_running;
    }
    else{
        current_running_s = current_running;
    }
    if(last_running->pgdir != current_running->pgdir){
        set_satp(SATP_MODE_SV39, current_running->pid, (kva2pa(current_running->pgdir)>>12));
        flush_all();
        //local_flush_tlb_all();
    }
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    //screen_cursor_x = current_running->cursor_x;
    //screen_cursor_y = current_running->cursor_y;
    // TODO: switch_to current_running
    //sbi_console_putstr("\n\nswitch_to\n");
    switch_to(last_running, next_running);
}



void do_sleep(uint32_t sleep_time)
{
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    current_running->status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout
    timer_creat(&do_unblock_timer, current_running, sleep_time *time_base);
    // 3. reschedule because the current_running is blocked.
    do_scheduler();
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    // TODO: block the pcb task into the block queue
    current_running->status = TASK_BLOCKED;
    add_queue(queue, pcb_node);
    do_scheduler();
}

void do_unblock(list_head *queue)
{
    // TODO: unblock the `pcb` from the block queue
    pcb_t * unblocked_task = delete_queue(queue);
    unblocked_task->status = TASK_READY;
    add_queue(&ready_queue, unblocked_task);
}

void do_unblock_timer(pcb_t *pcb_node){
    pcb_node->status = TASK_READY;
    add_queue(&ready_queue, pcb_node);
}

void add_queue(list_head* queue, pcb_t* item){
    list_add_tail(&item->list, queue);
}

pcb_t * delete_queue(list_head* queue){
    pcb_t* tmp = list_entry(queue->next, pcb_t, list);
    list_del(queue->next);
    return tmp;
}

void do_priority(uint32_t num){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    priority_t prior = (priority_t)num;
    assert(prior >= 0 && prior < MAX_PRIORITY);
    current_running->priority = prior;
}

uint32_t do_fork(void){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    pcb_t *np;
    int num;
    for(num = 0; num < process_num; num++){
        if(pcb[num].status == TASK_EXITED){
            np = &pcb[num];
            break;
        }
    }
    if(num == process_num){
        np = &pcb[process_num++];
    }

    *np = *current_running;
    //np->pgdir = current_running->pgdir;
    np->kernel_stack_base = allocPage(current_running) + PAGE_SIZE;
    current_running->child_num++;
    np->user_stack_base = USER_STACK_ADDR + current_running->child_num * PAGE_SIZE;
    uintptr_t src_pgdir = pa2kva(PGDIR_PA);
    flush_all();
    int i;

    //new pcb's information
    np->pid = process_id;
    np->status = TASK_READY;
    //np->kernel_sp = allocPage(1) + PAGE_SIZE;
    //np->user_sp = allocPage(1) + PAGE_SIZE;
    np->kernel_sp = np->kernel_stack_base;

    regs_context_t *pt_regs =
        (regs_context_t *) (np->kernel_sp - sizeof(regs_context_t));

//    switchto_context_t *st_regs =
//        (switchto_context_t *) (np->kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t));
    
    //copy data from father pcb
    kmemcpy(np->kernel_sp - PAGE_SIZE, 
            current_running->kernel_sp + sizeof(regs_context_t) + sizeof(switchto_context_t) - PAGE_SIZE, 
            PAGE_SIZE);
    //kmemcpy(np->user_sp - PAGE_SIZE, current_running->user_sp - PAGE_SIZE, PAGE_SIZE);

    np->kernel_sp = np->kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t);
    reg_t sp_distance = np->user_sp - current_running->user_sp;

    pt_regs->regs[10] = 0;                    //a0 chlid process return 0

    pt_regs->regs[2] += sp_distance;          //sp
    pt_regs->regs[8] += sp_distance;          //s0 
    pt_regs->regs[4] = (reg_t)np;             //tp

//    st_regs->regs[0] = (reg_t)ret_from_exception;   //ra

    //add into ready_queue
    add_queue(&ready_queue, np);

    return process_id++;
}

//shell command

void do_process_show(){
    prints("[PRCESS TABLE]\n");
    int i;
    int run_core;
    for(i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].pid == current_running_m->pid){
            run_core = 0;
        }
        else{
            run_core = 1;
        }
        if(pcb[i].pid)
        switch(pcb[i].status){
            case TASK_RUNNING:
                prints("[%d] PID : %d STATUS : %s MASK : 0x%d on Core %d\n", i, pcb[i].pid, "RUNNING", pcb[i].mask, run_core);
                break;
            case TASK_BLOCKED:
                prints("[%d] PID : %d STATUS : %s MASK : 0x%d\n", i, pcb[i].pid, "BLOCKED", pcb[i].mask);
                break;
            case TASK_EXITED:
                prints("[%d] PID : %d STATUS : %s MASK : 0x%d\n", i, pcb[i].pid, "EXITED", pcb[i].mask);
                break;
            case TASK_READY:
                prints("[%d] PID : %d STATUS : %s MASK : 0x%d\n", i, pcb[i].pid, "READY", pcb[i].mask);
                break;
            case TASK_ZOMBIE:
                prints("[%d] PID : %d STATUS : %s MASK : 0x%d\n", i, pcb[i].pid, "ZOMBIE", pcb[i].mask);
                break;
        }
    }
}


void do_show_exec(){
    for (int i = 0;i < NUM_PROGRAM; i++)
    prints("%s\n",elf_files[i].file_name);
}

pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    pcb_t *new_pcb;
    int num;
    for(num = 0; num < process_num; num++){
        if(pcb[num].status == TASK_EXITED){
            new_pcb = &pcb[num];
            new_pcb->kernel_sp = new_pcb->kernel_stack_base;
            new_pcb->user_sp = new_pcb->user_stack_base;
            break;
        }
    }
    if(num == process_num){
        new_pcb = &pcb[process_num++];
        new_pcb->kernel_sp = allocPage(1) + PAGE_SIZE;
        new_pcb->user_sp = allocPage(1) + PAGE_SIZE;
        new_pcb->kernel_stack_base = new_pcb->kernel_sp;
        new_pcb->user_stack_base = new_pcb->user_sp;
    }
    new_pcb->type = task->type;
    init_pcb_stack(new_pcb->kernel_sp, new_pcb->user_sp, task->entry_point, NULL, arg, new_pcb);
    new_pcb->pid = process_id++;
    new_pcb->status = TASK_READY;
    new_pcb->priority = PRIOR_1;
    new_pcb->mode = mode;
    new_pcb->cursor_x = 0;
    new_pcb->cursor_y = 0;
    new_pcb->time_label = get_ticks();
    new_pcb->lock_num = 0;
    new_pcb->mask = current_running->mask;
    add_queue(&ready_queue, new_pcb);
    init_list_head(&new_pcb->wait_list);
    return new_pcb->pid;
}

pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    pcb_t *new_pcb;
    int num;
    for(num = 0; num < process_num; num++){
        if(pcb[num].status == TASK_EXITED){
            new_pcb = &pcb[num];
            break;
        }
    }
    if(num == process_num){
        new_pcb = &pcb[process_num++];
    }
    new_pcb->page_num = 0;
    new_pcb->pgdir = allocPage(new_pcb);
    clear_pgdir(new_pcb->pgdir);
    new_pcb->kernel_stack_base = allocPage(new_pcb) + PAGE_SIZE;
    new_pcb->user_stack_base = USER_STACK_ADDR;
    uintptr_t src_pgdir = pa2kva(PGDIR_PA);
    share_pgtable(new_pcb->pgdir, src_pgdir);
    uintptr_t kva_stack = alloc_page_helper(new_pcb->user_stack_base - 0x1000, new_pcb->pgdir, new_pcb);
    int i;
    for(i = 0; i < NUM_MAX_PPAGE; i++){
        new_pcb->pa_table[i] = allocPage(new_pcb) - KPA_OFFSET;
    }
    for(i = 0;i < NUM_PROGRAM; i++){
        if(!kstrcmp(elf_files[i].file_name,file_name)){
            break;
        }
    }
    if(i == NUM_PROGRAM)
        return -1;

    unsigned file_len = *(elf_files[i].file_length);
    user_entry_t entry_point = (user_entry_t)load_elf(elf_files[i].file_content, file_len, new_pcb->pgdir, new_pcb, alloc_page_helper);
    flush_all();
    //local_flush_tlb_all();
    new_pcb->pid = process_id++;
    new_pcb->status = TASK_READY;
    new_pcb->type = USER_PROCESS;
    new_pcb->cursor_x = 1;
    new_pcb->cursor_y = 1;
    new_pcb->time_label = get_ticks();
    new_pcb->priority = PRIOR_1;
    new_pcb->mask = current_running->mask;
    new_pcb->mode = mode;
    new_pcb->kernel_sp = new_pcb->kernel_stack_base;
    //save space for argc, argv
    new_pcb->user_sp = new_pcb->user_stack_base - 0x200;
    new_pcb->father = new_pcb;
    new_pcb->child_num = 0;
    new_pcb->shm_page_num = 0;
    new_pcb->lock_num = 0;
    init_list_head(&new_pcb->wait_list);

    for(i = 0; i < NUM_MAX_VPAGE; i++){
        new_pcb->temp_page_table[i].block = 0;
        new_pcb->temp_page_table[i].in_mem = 0;
        new_pcb->temp_page_table[i].va = 0;
        new_pcb->temp_page_table[i].pa_ptr = 0;
    }

    //fetch argc & argv from user stack through linear map
    kva_stack = kva_stack + 0x1000 - 0x200;
    uintptr_t uva_argv = 0xf00010000 - 0x100;
    uintptr_t kva_argvi = kva_stack;
    uintptr_t uva_argvi = 0xf00010000 - 0x200;
    kva_stack += 0x100;
    init_pcb_stack(new_pcb->kernel_sp, new_pcb->user_sp, (ptr_t)entry_point, argc, (void *)uva_argv, new_pcb);
    for(i = 0; i < argc; i++){
        if(argv[i]==0){
            break;
        }
        kmemcpy((uint8_t *)(kva_argvi), (uint8_t *)argv[i], kstrlen(argv[i]) + 1);
        *(uintptr_t *)kva_stack = uva_argvi;
        kva_stack += 8;
        kva_argvi += kstrlen(argv[i]) + 1;
        uva_argvi += kstrlen(argv[i]) + 1;
    }
    list_add(&new_pcb->list, &ready_queue);
    return new_pcb->pid;
}

void do_exit(void){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    pcb_t *exit_pcb = current_running;
    //release wait list
    while(!list_empty(&exit_pcb->wait_list)){
        pcb_t *wait_pcb;
        wait_pcb = list_entry(exit_pcb->wait_list.prev, pcb_t, list);
        if(wait_pcb->status != TASK_EXITED){
            do_unblock(&(exit_pcb->wait_list));
        }
    }

    //release required lock
    int i;
    for(i = 0; i < exit_pcb->lock_num; i++){
        do_mutex_lock_release(exit_pcb->locks[i]);
    }

    //recycle pcb and memory according to pcb's mode
    exit_pcb->father->child_num--;
    if(exit_pcb->mode == AUTO_CLEANUP_ON_EXIT){
        exit_pcb->status = TASK_EXITED;
    }
    else{
        exit_pcb->status = TASK_ZOMBIE;
    }
    do_scheduler();
}

int do_kill(pid_t pid){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    pcb_t *kill_pcb;
    int i;
    for(i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].pid == pid)
            break;
    }
    if(i == NUM_MAX_TASK){
        return 0;
    }
    kill_pcb = &pcb[i];

    list_del(&kill_pcb->list); 
    //release wait list
    while(!list_empty(&kill_pcb->wait_list)){
        pcb_t *wait_pcb;
        wait_pcb = list_entry(kill_pcb->wait_list.prev, pcb_t, list);
        if(wait_pcb->status != TASK_EXITED){
            do_unblock(&kill_pcb->wait_list);
        }
    }

    //release required lock
    for(i = 0; i < kill_pcb->lock_num; i++){
        do_mutex_lock_release(kill_pcb->locks[i]);
    }

    //recycle pcb and memory according to pcb's mode
    kill_pcb->father->child_num--;
    if(kill_pcb->mode == AUTO_CLEANUP_ON_EXIT){
        kill_pcb->status = TASK_EXITED;
    }
    else{
        kill_pcb->status = TASK_ZOMBIE;
    }

    if(current_running->pid == kill_pcb->pid){
        do_scheduler();
    }
}

int do_waitpid(pid_t pid){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    int i;
    for(i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].pid == pid)
            break;
    }
    if(i == NUM_MAX_TASK){
        return 0;
    }
    if(pcb[i].status != TASK_EXITED && pcb[i].status != TASK_ZOMBIE)
        do_block(current_running, &(pcb[i].wait_list));
    return 1;
}

pid_t do_getpid(){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    return current_running->pid;
}

//synchronization primitives
//semaphore
int semaphore_count = 0;
semaphore_t SEMAPHORE_LIST[MAX_SEMAPHORE_NUM];
int semaphore_init(int val){
    int i;
    for(i = 0; i < semaphore_count; i++){
        if(SEMAPHORE_LIST[i].destoryed == 1){
            SEMAPHORE_LIST[semaphore_count].val = val;
            init_list_head(&(SEMAPHORE_LIST[semaphore_count].block_queue));
            SEMAPHORE_LIST[semaphore_count].destoryed = 0;
            return i;
        }
    }
    semaphore_count++;
    SEMAPHORE_LIST[semaphore_count].val = val;
    init_list_head(&(SEMAPHORE_LIST[semaphore_count].block_queue));
    SEMAPHORE_LIST[semaphore_count].destoryed = 0;
    return semaphore_count;
}

int semaphore_up(int handle){
    if(SEMAPHORE_LIST[handle].destoryed){
        return handle;
    }
    if(!list_empty(&(SEMAPHORE_LIST[handle].block_queue))){
        do_unblock(&(SEMAPHORE_LIST[handle].block_queue));
    }
    else{
        SEMAPHORE_LIST[handle].val++;
    }
    return handle;
}

int semaphore_down(int handle){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(SEMAPHORE_LIST[handle].destoryed){
        return handle;
    }
    if(SEMAPHORE_LIST[handle].val > 0){
        SEMAPHORE_LIST[handle].val--;
    }
    else{
        do_block(current_running, &(SEMAPHORE_LIST[handle].block_queue));
        while(SEMAPHORE_LIST[handle].val <= 0 && !SEMAPHORE_LIST[handle].destoryed) ;
        SEMAPHORE_LIST[handle].val--;
    }
    return handle;
}

int semaphore_destory(int handle){
    SEMAPHORE_LIST[handle].val = 0;
    SEMAPHORE_LIST[semaphore_count].destoryed = 1;
    while(!list_empty(&(SEMAPHORE_LIST[handle].block_queue))){
        do_unblock(&(SEMAPHORE_LIST[handle].block_queue));
    }
    init_list_head(&(SEMAPHORE_LIST[handle].block_queue));
    return handle;
}
//barrier
int barrier_count = 0;
barrier_t BARRIER_LIST[MAX_BARRIER_NUM];
int barrier_init(int count){
    barrier_count++;
    BARRIER_LIST[barrier_count].total_num = count;
    BARRIER_LIST[barrier_count].wait_num = 0;
    BARRIER_LIST[barrier_count].destoryed = 0;
    init_list_head(&(BARRIER_LIST[barrier_count].block_queue));
    return barrier_count;
}

int barrier_wait(int handle){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    if(BARRIER_LIST[handle].destoryed){
        return handle;
    }
    BARRIER_LIST[handle].wait_num++;
    if(BARRIER_LIST[handle].wait_num == BARRIER_LIST[handle].total_num){
        while(!list_empty(&(BARRIER_LIST[barrier_count].block_queue))){
            do_unblock(&(BARRIER_LIST[barrier_count].block_queue));
        }
        BARRIER_LIST[handle].wait_num = 0;
    }
    else{
        do_block(current_running, &(BARRIER_LIST[handle].block_queue));
        //while(BARRIER_LIST[handle].wait_num != BARRIER_LIST[handle].total_num && !BARRIER_LIST[handle].destoryed) ;
    }
    return handle;
}

int barrier_destory(int handle){
    BARRIER_LIST[handle].total_num = 0;
    BARRIER_LIST[handle].wait_num = 0;
    BARRIER_LIST[handle].destoryed = 1;
    while(!list_empty(&(BARRIER_LIST[barrier_count].block_queue))){
        do_unblock(&(BARRIER_LIST[barrier_count].block_queue));
    }
    return handle;
}

mailbox_k_t MBOX_LIST[MAX_MBOX_NUM];
int mbox_count = 0;
int mbox_open_k(char *name){
    int i;
    for(i = 0; i < MAX_MBOX_NUM; i++){
        if((kstrcmp(name, MBOX_LIST[i].name) == 0) && (MBOX_LIST[i].status = MBOX_OPEN)){
            return i;
        }
    }
    
    for(i = 1; i < mbox_count; i++){
        if(MBOX_LIST[i].status == MBOX_CLOSE){
            MBOX_LIST[i].status = MBOX_OPEN;
            MBOX_LIST[mbox_count].index = 0;
            init_list_head(&(MBOX_LIST[mbox_count].full_queue));
            init_list_head(&(MBOX_LIST[mbox_count].empty_queue));
            int j = 0;
            while(*name){
                MBOX_LIST[i].name[j++]=*name;
                name++;
            }
            MBOX_LIST[i].name[j]='\0';
            return i;
        }
    }
    mbox_count++;
    MBOX_LIST[mbox_count].status = MBOX_OPEN;
    MBOX_LIST[mbox_count].index = 0;
    init_list_head(&(MBOX_LIST[mbox_count].full_queue));
    init_list_head(&(MBOX_LIST[mbox_count].empty_queue));
    int j = 0;
    while(*name){
        MBOX_LIST[mbox_count].name[j++] = *name;
        name++;
    }
    MBOX_LIST[mbox_count].name[j]='\0';
    return mbox_count;
}

void mbox_close_k(int handle){
    MBOX_LIST[handle].status = MBOX_CLOSE;
}

int mbox_send_k(int handle, void *msg, int msg_length){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    int blocked = 0;
    while(MBOX_LIST[handle].index + msg_length > MAX_MBOX_LENGTH){
        do_block(current_running, &(MBOX_LIST[handle].full_queue));
        //while(MBOX_LIST[handle].index + msg_length > MAX_MBOX_LENGTH) ;
        blocked = 1;
    }
    int i;
    for(i = 0; i < msg_length; i++){
        MBOX_LIST[handle].msg[MBOX_LIST[handle].index++] = ((char *)msg)[i];
    }
    while(!list_empty(&(MBOX_LIST[handle].empty_queue))){
        do_unblock(&(MBOX_LIST[handle].empty_queue));
    }
    return blocked;
}

int mbox_recv_k(int handle, void *msg, int msg_length){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    int blocked = 0;
    while(MBOX_LIST[handle].index - msg_length < 0){
        do_block(current_running, &(MBOX_LIST[handle].empty_queue));
        //while(MBOX_LIST[handle].index - msg_length < 0);
        blocked = 1;
    }
    int i;
    for(i = 0; i < msg_length; i++){
        ((char *)msg)[i] = MBOX_LIST[handle].msg[i];
    }
    for(i = 0; i < MAX_MBOX_LENGTH - msg_length; i++){
        MBOX_LIST[handle].msg[i] = MBOX_LIST[handle].msg[i + msg_length];
    }
    MBOX_LIST[handle].index -= msg_length;
    while(!list_empty(&(MBOX_LIST[handle].full_queue))){
        do_unblock(&(MBOX_LIST[handle].full_queue));
    }
    return blocked;
}

int mbox_send_recv_k(int send, int recv, void *msg){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    int flag;
    int length = 0;
    char *ch = msg;
    while(*ch != '\0'){
        length++;
        ch++;
    }
empty:
    if(MBOX_LIST[recv].index > 0){
        flag = MBOX_LIST[recv].index << 2;
        flag += 0x2;
        while(!list_empty(&(MBOX_LIST[recv].full_queue))){
            do_unblock(&(MBOX_LIST[recv].full_queue));
        }
        MBOX_LIST[recv].index = 0;
        return flag;
    }
    else{
        while(MBOX_LIST[send].index + length > MAX_MBOX_LENGTH){
            if(MBOX_LIST[recv].index > 0){
                goto empty;
            }
            do_block(current_running, &(MBOX_LIST[send].full_queue));
        }
        flag = length << 2;
        flag += 0x1;
        int i;
        for(i = 0; i < length; i++){
            MBOX_LIST[send].msg[MBOX_LIST[send].index++] = ((char *)msg)[i];
        }
        while(!list_empty(&(MBOX_LIST[send].empty_queue))){
            do_unblock(&(MBOX_LIST[send].empty_queue));
        }
        return flag;
    }
}

void do_taskset_p(int mask, pid_t pid){
    int i;
    for(i = 0; (pcb[i].pid != pid) && i < NUM_MAX_TASK; i++);
    if (i == NUM_MAX_TASK){
        return;
    }
    pcb_t *taskset_pcb = &pcb[i];
    taskset_pcb->mask = mask;
}


void do_taskset_spawn(int mask, task_info_t *task, spawn_mode_t mode){
    pcb_t *new_pcb;
    int num;
    for(num = 0; num < process_num; num++){
        if(pcb[num].status == TASK_EXITED){
            new_pcb = &pcb[num];
            new_pcb->kernel_sp = new_pcb->kernel_stack_base;
            new_pcb->user_sp = new_pcb->user_stack_base;
            break;
        }
    }
    if(num == process_num){
        new_pcb = &pcb[process_num++];
        new_pcb->kernel_sp = allocPage(1) + PAGE_SIZE;
        new_pcb->user_sp = allocPage(1) + PAGE_SIZE;
        new_pcb->kernel_stack_base = new_pcb->kernel_sp;
        new_pcb->user_stack_base = new_pcb->user_sp;
    }
    new_pcb->type = task->type;
    init_pcb_stack(new_pcb->kernel_sp, new_pcb->user_sp, task->entry_point, NULL, NULL, new_pcb);
    new_pcb->pid = process_id++;
    new_pcb->status = TASK_READY;
    new_pcb->priority = PRIOR_1;
    new_pcb->mode = mode;
    new_pcb->cursor_x = 0;
    new_pcb->cursor_y = 0;
    new_pcb->time_label = get_ticks();
    new_pcb->lock_num = 0;
    new_pcb->mask = mask;
    add_queue(&ready_queue, new_pcb);
    init_list_head(&new_pcb->wait_list);
}

//mthread
int do_mthread_create(mthread_t *thread, void (*start_routine)(void*), void *arg){
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    pcb_t *new_pcb;
    int num;
    for(num = 0; num < process_num; num++){
        if(pcb[num].status == TASK_EXITED){
            new_pcb = &pcb[num];
            break;
        }
    }
    if(num == process_num){
        new_pcb = &pcb[process_num++];
    }
    new_pcb->page_num = 0;
    new_pcb->pgdir = current_running->pgdir;
    new_pcb->kernel_stack_base = allocPage(current_running) + PAGE_SIZE;
    current_running->child_num++;
    new_pcb->user_stack_base = USER_STACK_ADDR + current_running->child_num * PAGE_SIZE;
    uintptr_t src_pgdir = pa2kva(PGDIR_PA);
    flush_all();
    int i;
    //local_flush_tlb_all();
    new_pcb->pid = process_id++;
    new_pcb->status = TASK_READY;
    new_pcb->type = USER_THREAD;
    new_pcb->cursor_x = 1;
    new_pcb->cursor_y = 1;
    new_pcb->time_label = get_ticks();
    new_pcb->priority = current_running->priority;
    new_pcb->mask = current_running->mask;
    new_pcb->mode = current_running->mode;
    new_pcb->kernel_sp = new_pcb->kernel_stack_base;
    new_pcb->user_sp = new_pcb->user_stack_base;
    new_pcb->father = current_running;
    new_pcb->child_num = 0;
    new_pcb->shm_page_num = current_running->shm_page_num;
    new_pcb->lock_num = 0;
    init_list_head(&new_pcb->wait_list);

    for(i = 0; i < NUM_MAX_VPAGE; i++){
        new_pcb->temp_page_table[i].block = 0;
        new_pcb->temp_page_table[i].in_mem = 0;
        new_pcb->temp_page_table[i].va = 0;
        new_pcb->temp_page_table[i].pa_ptr = 0;
    }
    for(i = 0; i < NUM_MAX_PPAGE; i++){
        new_pcb->pa_table[i] = current_running->pa_table[i];
    }

    init_pcb_stack(new_pcb->kernel_sp, new_pcb->user_sp, (ptr_t)start_routine, arg, NULL, new_pcb);
    regs_context_t *spt_regs = (regs_context_t *)(new_pcb->kernel_stack_base - sizeof(regs_context_t));
    regs_context_t *fpt_regs = (regs_context_t *)(current_running->kernel_stack_base - sizeof(regs_context_t));
    spt_regs->regs[3] = fpt_regs->regs[3];  
    list_add(&new_pcb->list, &ready_queue);
    *thread = new_pcb->pid;
    return new_pcb->pid;
}
