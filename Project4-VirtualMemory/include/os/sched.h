/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_
 
//#include <context.h>

#include <type.h>
#include <os/list.h>
//#include <os/mm.h>
#include <os/time.h>
#include <os/lock.h>

#define NUM_MAX_TASK 64
#define MAX_PRIORITY 5
#define MAX_SEMAPHORE_NUM 16
#define MAX_BARRIER_NUM 16
#define MAX_MBOX_NUM 16
#define MAX_MBOX_LENGTH (64)
#define NUM_MAX_PPAGE 4
#define NUM_MAX_VPAGE 512

/* used to save register infomation */
typedef struct regs_context
{
    /* Saved main processor registers.*/
    reg_t regs[32];

    /* Saved special registers. */
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;

/* used to save register infomation in switch_to */
typedef struct switchto_context
{
    /* Callee saved registers.*/
    reg_t regs[14];
} switchto_context_t;

typedef void (*user_entry_t)(unsigned long,unsigned long,unsigned long);

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;

typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

typedef enum{
    PRIOR_0,
    PRIOR_1,
    PRIOR_2,
    PRIOR_3,
    PRIOR_4,
}priority_t;

typedef struct page_info
{
    int in_mem;
    uintptr_t va;
    int block;
    int pa_ptr;
} page_info_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    /* previous, next pointer */
    list_node_t list;

    /* tmp user_sp (used in save context)*/
    reg_t save_sp;

    /* process id */
    pid_t pid;

    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING | ZOMBIE */
    task_status_t status;
    spawn_mode_t mode;

    /* cursor position */
    int cursor_x;
    int cursor_y;

    //timer
    timer_t timer;

    //priority
    priority_t priority;
    uint64_t time_label;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    list_head wait_list;

    /* locks pcb acquired */
    int lock_num;
    mutex_lock_t *locks[10];

    /* can run on witch core*/
    int mask;

    //pvaddr
    uintptr_t pgdir;
    ptr_t page_table[3*NUM_MAX_VPAGE];
    int page_num;
    struct pcb* father;
    int child_num;
    int temp_page_num;
    page_info_t temp_page_table[NUM_MAX_VPAGE];
    uintptr_t pa_table[NUM_MAX_PPAGE];
    //share mem
    int shm_page_num;

} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    ptr_t entry_point;
    task_type_t type;
} task_info_t;

//primitive
typedef struct semaphore
{
    int val;
    list_head block_queue;
    int destoryed;
} semaphore_t;

typedef struct barrier
{
    int total_num;
    int wait_num;
    list_head block_queue;
    int destoryed;
} barrier_t;

typedef enum {
    MBOX_CLOSE,
    MBOX_OPEN,
} mailbox_status_t;
typedef struct mailbox_k
{
    char name[25];                   //name of mailbox
    char msg[MAX_MBOX_LENGTH];       //content of message
    int index;                       //current length of msg
    mailbox_status_t status;
    list_head empty_queue;
    list_head full_queue;
    int semaphore_id;
} mailbox_k_t;

/* ready queue to run */
extern list_head ready_queue;

//sleep queue
extern list_head sleep_queue;

/* current running task PCB */
extern pcb_t * volatile current_running;
extern pcb_t * volatile current_running_m;
extern pcb_t * volatile current_running_s;

extern ptr_t stack_table[512];
extern short stack_index;
// extern pcb_t * volatile current_running[NR_CPUS];
extern pid_t process_id;
extern int process_num;

extern pcb_t pcb[NUM_MAX_TASK];
// extern pcb_t kernel_pcb[NR_CPUS];
extern pcb_t pid0_pcb_m;
extern const ptr_t pid0_stack_m;
extern pcb_t pid0_pcb_s;
extern const ptr_t pid0_stack_s;

extern void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, int arg, void* argv, pcb_t *pcb);
    
extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);
void do_priority(uint32_t);
uint32_t do_fork(void);

void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *);
extern pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();
extern void do_taskset_p(int mask, pid_t pid);
extern void do_taskset_spawn(int mask, task_info_t *task, spawn_mode_t mode);
extern pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode);
extern void do_show_exec();
 

void do_unblock_timer(pcb_t *);

void add_queue(list_head* queue, pcb_t* item);
pcb_t * delete_queue(list_head* queue);

extern int semaphore_init(int val);
extern int semaphore_up(int handle);
extern int semaphore_down(int handle);
extern int semaphore_destory(int handle);
extern int barrier_init(int count);
extern int barrier_wait(int handle);
extern int barrier_destory(int handle);
extern int mbox_open_k(char *name);
extern void mbox_close_k(int handle);
extern int mbox_send_k(int handle, void *msg, int msg_length);
extern int mbox_recv_k(int handle, void *msg, int msg_length);
extern int mbox_send_recv_k(int send, int recv, void *msg);

typedef pid_t mthread_t;
extern int do_mthread_create(mthread_t *thread,void (*start_routine)(void*),void *arg);
#endif