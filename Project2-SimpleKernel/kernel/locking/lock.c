#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

#define MUTEX_LOCK_NUMBER 10

mutex_lock_t MUTEX_LOCK[MUTEX_LOCK_NUMBER];
int count;

int mutex_get(void)
{
    count++;
    do_mutex_lock_init(&MUTEX_LOCK[count]);
    return count;
}

int mutex_lock(int handle)
{
    do_mutex_lock_acquire(&MUTEX_LOCK[handle]);
    return handle;
}

int mutex_unlock(int handle)
{
    do_mutex_lock_release(&MUTEX_LOCK[handle]);
    return handle;
}

void do_mutex_lock_init(mutex_lock_t *lock)
{
    /* TODO */
    lock->lock.status = UNLOCKED;
    init_list_head(&lock->block_queue);
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    /* TODO */
    if(lock->lock.status == UNLOCKED){
      lock->lock.status = LOCKED;
    }else{
      do_block(current_running, &lock->block_queue);
    }
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    /* TODO */
    if(list_empty(&lock->block_queue)){
      lock->lock.status = UNLOCKED;
    }else{
      do_unblock(&lock->block_queue);
    }
}
