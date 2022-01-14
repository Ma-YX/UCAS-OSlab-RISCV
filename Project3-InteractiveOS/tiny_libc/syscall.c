#include <sys/syscall.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    // TODO:
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    // TODO:
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
    //   or
    //do_scheduler();
    // ???
}

int sys_mutex_get(void){
    return invoke_syscall(SYSCALL_MUTEX_GET, IGNORE, IGNORE, IGNORE);
}

int sys_mutex_lock(int handle){
    return invoke_syscall(SYSCALL_MUTEX_LOCK, handle, IGNORE, IGNORE);
}

int sys_mutex_unlock(int handle){
    return invoke_syscall(SYSCALL_MUTEX_UNLOCK, handle, IGNORE, IGNORE);
}

char sys_read(void){
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE);
}

void sys_priority(uint32_t priority){
    invoke_syscall(SYSCALL_PRIORITY, priority, IGNORE, IGNORE);
}

int sys_get_char(void){
    return invoke_syscall(SYSCALL_GET_CHAR, IGNORE, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode);
}

void sys_exit(void){
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid){
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid){
    return invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE);
}

void sys_process_show(void){
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void){
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid(){
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE);
}

void sys_serial_write(char c){
    invoke_syscall(SYSCALL_SERIAL_WRITE, c, IGNORE, IGNORE);
}

//synchronization primitives
//semaphore
int sys_semaphore_init(int val){
    return invoke_syscall(SYSCALL_SEMAPHORE_INIT, val, IGNORE, IGNORE);
}

int sys_semaphore_up(int handle){
    return invoke_syscall(SYSCALL_SEMAPHORE_UP, handle, IGNORE, IGNORE);
}

int sys_semaphore_down(int handle){
    return invoke_syscall(SYSCALL_SEMAPHORE_DOWN, handle, IGNORE, IGNORE);
}

int sys_semaphore_destory(int handle){
    return invoke_syscall(SYSCALL_SEMAPHORE_DESTORY, handle, IGNORE, IGNORE);
}
//barrier
int sys_barrier_init(int count){
    return invoke_syscall(SYSCALL_BARRIER_INIT, count, IGNORE, IGNORE);
}

int sys_barrier_wait(int handle){
    return invoke_syscall(SYSCALL_BARRIER_WAIT, handle, IGNORE, IGNORE);
}

int sys_barrier_destory(int handle){
    return invoke_syscall(SYSCALL_BARRIER_DESTORY, handle, IGNORE, IGNORE);
}

int sys_mbox_open(char *name){
    return invoke_syscall(SYSCALL_MBOX_OPEN, (uintptr_t)name, IGNORE, IGNORE);
}

void sys_mbox_close(int handle){
    invoke_syscall(SYSCALL_MBOX_CLOSE, handle, IGNORE, IGNORE);
}

int sys_mbox_send(int handle, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_SEND, handle, (uintptr_t)msg, msg_length);
}

int sys_mbox_recv(int handle, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_RECV, handle, (uintptr_t)msg, msg_length);
}

int sys_mbox_send_recv(int send, int recv, void *msg){
    return invoke_syscall(SYSCALL_MBOX_SEND_RECV, send, recv, msg);
}

void sys_taskset_p(int mask, pid_t pid){
    invoke_syscall(SYSCALL_TASKSET_P, mask, pid, IGNORE);
}

void sys_taskset_spawn(int mask, task_info_t *info, spawn_mode_t mode){
    invoke_syscall(SYSCALL_TASKSET_SPAWN, mask, info, mode);
}