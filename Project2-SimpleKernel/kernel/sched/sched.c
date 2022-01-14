#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>

//#define RR
#define PRIORITY

pcb_t pcb[NUM_MAX_TASK];
const ptr_t pid0_stack = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack,
    .user_sp = (ptr_t)pid0_stack,
    .preempt_count = 0
};

LIST_HEAD(ready_queue);
void add_queue(list_head* queue, pcb_t* item);
pcb_t *delete_queue(list_head* queue);

/* current running task PCB */
pcb_t * volatile current_running;

/* global process id */
pid_t process_id = 1;

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
    #ifdef RR
    pcb_t *next_running = delete_queue(&ready_queue);
    #endif
    #ifdef PRIORITY
    pcb_t *next_running = prior_search_next(&ready_queue);
    list_del(&next_running->list);
    next_running->time_label = get_ticks();
    #endif
    pcb_t * last_running = current_running;
    if(current_running->status == TASK_RUNNING && current_running->pid != 0){
        current_running->status = TASK_READY;
        add_queue(&ready_queue, current_running);
    }
    next_running->status = TASK_RUNNING;
    current_running = next_running;
    //process_id = current_running->pid;
    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running->cursor_x,
                      current_running->cursor_y);
    screen_cursor_x = current_running->cursor_x;
    screen_cursor_y = current_running->cursor_y;
    // TODO: switch_to current_running
    switch_to(last_running, next_running);
}

void do_sleep(uint32_t sleep_time)
{
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
    do_scheduler();
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
    priority_t prior = (priority_t)num;
    assert(prior >= 0 && prior < MAX_PRIORITY && "Priority should be between 0 and 4 !");
    current_running->priority = prior;
}

uint32_t fork(void){
    assert(process_id < NUM_MAX_TASK && "TOO MUCH TASKS (16 at most) !");
    pcb_t *np = pcb + process_id;
    *np = *current_running;

    //new pcb's information
    np->pid = process_id;
    np->status = TASK_READY;
    np->kernel_sp = allocPage(1) + PAGE_SIZE;
    np->user_sp = allocPage(1) + PAGE_SIZE;

    regs_context_t *pt_regs =
        (regs_context_t *) (np->kernel_sp - sizeof(regs_context_t));

//    switchto_context_t *st_regs =
//        (switchto_context_t *) (np->kernel_sp - sizeof(regs_context_t) - sizeof(switchto_context_t));
    
    //copy data from father pcb
    kmemcpy(np->kernel_sp - PAGE_SIZE, 
            current_running->kernel_sp + sizeof(regs_context_t) + sizeof(switchto_context_t) - PAGE_SIZE, 
            PAGE_SIZE);
    kmemcpy(np->user_sp - PAGE_SIZE, current_running->user_sp - PAGE_SIZE, PAGE_SIZE);

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