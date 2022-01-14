#include <sys/syscall.h>
#include <stdint.h>
#include <mthread.h>
#include <sys/shm.h>

void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    // TODO:
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
    //   or
    //do_scheduler();
    // ???
}

int sys_mutex_get(int key){
    return invoke_syscall(SYSCALL_MUTEX_GET, key, IGNORE, IGNORE, IGNORE);
}

int sys_mutex_lock(int handle){
    return invoke_syscall(SYSCALL_MUTEX_LOCK, handle, IGNORE, IGNORE, IGNORE);
}

int sys_mutex_unlock(int handle){
    return invoke_syscall(SYSCALL_MUTEX_UNLOCK, handle, IGNORE, IGNORE, IGNORE);
}

char sys_read(void){
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_priority(uint32_t priority){
    invoke_syscall(SYSCALL_PRIORITY, priority, IGNORE, IGNORE, IGNORE);
}

int sys_get_char(void){
    return invoke_syscall(SYSCALL_GET_CHAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char *argv[], spawn_mode_t mode){
    return invoke_syscall(SYSCALL_EXEC, (uintptr_t)file_name,argc, (uintptr_t)argv, mode);
}

void sys_show_exec(){
    invoke_syscall(SYSCALL_LS, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode, IGNORE);
}

void sys_exit(void){
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid){
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid){
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

void sys_process_show(void){
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void){
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid(){
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_serial_write(char c){
    invoke_syscall(SYSCALL_SERIAL_WRITE, c, IGNORE, IGNORE, IGNORE);
}

//synchronization primitives
//semaphore
int sys_semaphore_init(int val){
    return invoke_syscall(SYSCALL_SEMAPHORE_INIT, val, IGNORE, IGNORE, IGNORE);
}

int sys_semaphore_up(int handle){
    return invoke_syscall(SYSCALL_SEMAPHORE_UP, handle, IGNORE, IGNORE, IGNORE);
}

int sys_semaphore_down(int handle){
    return invoke_syscall(SYSCALL_SEMAPHORE_DOWN, handle, IGNORE, IGNORE, IGNORE);
}

int sys_semaphore_destory(int handle){
    return invoke_syscall(SYSCALL_SEMAPHORE_DESTORY, handle, IGNORE, IGNORE, IGNORE);
}
//barrier
int sys_barrier_init(int count){
    return invoke_syscall(SYSCALL_BARRIER_INIT, count, IGNORE, IGNORE, IGNORE);
}

int sys_barrier_wait(int handle){
    return invoke_syscall(SYSCALL_BARRIER_WAIT, handle, IGNORE, IGNORE, IGNORE);
}

int sys_barrier_destory(int handle){
    return invoke_syscall(SYSCALL_BARRIER_DESTORY, handle, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_open(char *name){
    return invoke_syscall(SYSCALL_MBOX_OPEN, (uintptr_t)name, IGNORE, IGNORE, IGNORE);
}

void sys_mbox_close(int handle){
    invoke_syscall(SYSCALL_MBOX_CLOSE, handle, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_send(int handle, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_SEND, handle, (uintptr_t)msg, msg_length, IGNORE);
}

int sys_mbox_recv(int handle, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_RECV, handle, (uintptr_t)msg, msg_length, IGNORE);
}

int sys_mbox_send_recv(int send, int recv, void *msg){
    return invoke_syscall(SYSCALL_MBOX_SEND_RECV, send, recv, msg, IGNORE);
}

void sys_taskset_p(int mask, pid_t pid){
    invoke_syscall(SYSCALL_TASKSET_P, mask, pid, IGNORE, IGNORE);
}

void sys_taskset_spawn(int mask, task_info_t *info, spawn_mode_t mode){
    invoke_syscall(SYSCALL_TASKSET_SPAWN, mask, info, mode, IGNORE);
}

int binsemget(int key){
    return invoke_syscall(SYSCALL_MUTEX_GET, IGNORE, IGNORE, IGNORE, IGNORE);
}

int binsemop(int binsem_id, int op){
    if(op == BINSEM_OP_LOCK){
        return invoke_syscall(SYSCALL_MUTEX_LOCK, binsem_id, IGNORE, IGNORE, IGNORE);
    }
    else if(op == BINSEM_OP_UNLOCK){
        return invoke_syscall(SYSCALL_MUTEX_UNLOCK, binsem_id, IGNORE, IGNORE, IGNORE);
    }
}

int mthread_create(mthread_t *thread, void (*start_routine)(void*), void *arg){
    return invoke_syscall(SYSCALL_MTHREAD_CREATE, thread, start_routine, arg, IGNORE);
}

int mthread_join(mthread_t thread){
    return invoke_syscall(SYSCALL_MTHREAD_JOIN, thread, IGNORE, IGNORE, IGNORE);
}

void *shmpageget(int key){
    return invoke_syscall(SYSCALL_SHM_PAGE_GET, key, IGNORE, IGNORE, IGNORE);
}

void shmpagedt(void *addr){
    return invoke_syscall(SYSCALL_SHM_PAGE_DT, addr, IGNORE, IGNORE, IGNORE);
}


long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength){
    return invoke_syscall(SYSCALL_NET_RECV, addr, length, num_packet, frLength);
}

void sys_net_send(uintptr_t addr, size_t length){
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}

void sys_net_irq_mode(int mode){
    invoke_syscall(SYSCALL_NET_IRQ_MODE, mode, IGNORE, IGNORE, IGNORE);
}