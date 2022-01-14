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

int mthread_barrier_init(mthread_barrier_t* handle, int count)
{
    // TODO:
    handle->id = sys_barrier_init(count);
    return handle->id;
}
int mthread_barrier_wait(mthread_barrier_t* handle)
{
    // TODO:
    return sys_barrier_wait(handle->id);
}
int mthread_barrier_destroy(mthread_barrier_t* handle)
{
    // TODO:
    return sys_barrier_destory(handle->id);
}

int mthread_semaphore_init(mthread_semaphore_t* handle, int val)
{
    // TODO:
    handle->id = sys_semaphore_init(val);
    return handle->id;
}
int mthread_semaphore_up(mthread_semaphore_t *handle)
{
    // TODO:
    return sys_semaphore_up(handle->id);
}
int mthread_semaphore_down(mthread_semaphore_t *handle)
{
    // TODO:
    return sys_semaphore_down(handle->id);
}
int mthread_semaphore_destroy(mthread_semaphore_t *handle)
{
    // TODO:
    return sys_semaphore_destory(handle->id);
}
