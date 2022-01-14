#include <sys/syscall.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdatomic.h>
#include <mthread.h>
#include <assert.h>

#define SHMP_KEY 37
#define NUM_CONSENSUS 8
#define LOCK2_BINSEM_KEY 42
#define ADD_NUM 500

int main(){
    int lock_id = sys_mutex_get(LOCK2_BINSEM_KEY);
    unsigned long long *result;
    result = (unsigned long long *)shmpageget(SHMP_KEY);
    mthread_barrier_t *barrier = (mthread_barrier_t *)shmpageget(SHMP_KEY + 1);
    int barrier_id = barrier->id;
    sys_barrier_wait(barrier_id);
    long long i;
    for(i = 0; i < ADD_NUM; i++){
        sys_mutex_lock(lock_id);
        *result = *result + i;
        sys_mutex_unlock(lock_id);
    }
    shmpagedt(result);
    shmpagedt(barrier);
}