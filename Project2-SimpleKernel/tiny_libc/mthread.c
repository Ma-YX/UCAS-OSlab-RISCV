#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>


int mthread_mutex_init(mthread_mutex_t *mutex)
{
    /* TODO: */
    mutex->id = sys_mutex_get();
}
int mthread_mutex_lock(mthread_mutex_t *mutex) 
{
    /* TODO: */
    return sys_mutex_lock(mutex->id);
    
}
int mthread_mutex_unlock(mthread_mutex_t *mutex)
{
    /* TODO: */
    return sys_mutex_unlock(mutex->id);
}
