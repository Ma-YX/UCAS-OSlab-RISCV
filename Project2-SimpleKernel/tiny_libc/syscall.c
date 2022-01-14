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

char sys_read(){
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE);
}

void sys_priority(uint32_t priority){
    invoke_syscall(SYSCALL_PRIORITY, priority, IGNORE, IGNORE);
}
