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

unsigned long long result_for_thread = 0;
mthread_barrier_t barrier_for_thread;
int thread_lock = 0;

uint32_t atomic_cmpxchg(uint32_t old_val, uint32_t new_val, ptr_t mem_addr)
{
    uint32_t ret;
    register unsigned int __rc;
    __asm__ __volatile__ (
          "0:	lr.w %0, %2\n"	
          "	bne  %0, %z3, 1f\n"
          "	sc.w.rl %1, %z4, %2\n"
          "	bnez %1, 0b\n"
          "	fence rw, rw\n"
          "1:\n"
          : "=&r" (ret), "=&r" (__rc), "+A" (*(uint32_t*)mem_addr)
          : "rJ" (old_val), "rJ" (new_val)
          : "memory");
    return ret;
}

void add_thread(){
    sys_barrier_wait(barrier_for_thread.id);
    long long i;
    for(i = 0; i < ADD_NUM; i++){
        while(atomic_cmpxchg(0, 1, &thread_lock))
            ;
        result_for_thread += i;
        thread_lock = 0;
    }
    int tmp = 0;
    sys_exit();
}

int main(int argc, char *argv[]){
    char *prog_name = argv[0];
    int print_location = 1;
    if (argc > 1) {
        print_location = atol(argv[1]);
    }
    sys_move_cursor(1, print_location);
    printf("process add begin\n");

    int lock_id = sys_mutex_get(LOCK2_BINSEM_KEY);
    unsigned long long *result = (unsigned long long *)shmpageget(SHMP_KEY);
    *result = 0;
    mthread_barrier_t *barrier = (mthread_barrier_t *)shmpageget(SHMP_KEY + 1);
    barrier->id = sys_barrier_init(2);
    int barrier_id = barrier->id;
    char process_add[15] = "processadd";
    int pid = sys_exec(&process_add, 0, &process_add, 1);
    printf("exec process add pid: %d\n", pid);

    sys_barrier_wait(barrier_id);
    unsigned long long start_ticks = sys_get_tick();
    printf("reach barrier\n");

    long long i;
    for(i = 0; i < ADD_NUM; i++){
        sys_mutex_lock(lock_id);
        *result += i;
        //printf("%lu ", *result);
        sys_mutex_unlock(lock_id);
    }
    sys_waitpid(pid);
    unsigned long long end_ticks = sys_get_tick();
    unsigned long long process_ticks = end_ticks - start_ticks;
    printf("process add result: %lu, ticks：%lu\n", *result, process_ticks);

    int tid;
    int barrier_for_thread_id = sys_barrier_init(2);
    barrier_for_thread.id = barrier_for_thread_id;
    printf("thread add begin\n");
    mthread_create(&tid, add_thread, 0);
    printf("exec thread add tid: %d\n", tid);
    sys_barrier_wait(barrier_for_thread_id);
    start_ticks = sys_get_tick();
    long long j;
    for(j = 0; j < ADD_NUM; j++){
        while(atomic_cmpxchg(0, 1, &thread_lock))
            ;
        result_for_thread += j;
        thread_lock = 0;
    }
    sys_waitpid(tid);
    end_ticks = sys_get_tick();
    unsigned long long thread_ticks = end_ticks - start_ticks;
    printf("thread add result: %lu, ticks：%lu\n", result_for_thread, thread_ticks);

    unsigned long long time = process_ticks / thread_ticks;
    printf("process / thread = %lu\n", time);

    shmpagedt(barrier);
    shmpagedt(result);
}