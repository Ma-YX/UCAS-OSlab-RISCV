#include <stdio.h>
#include <sys/syscall.h>
#include <assert.h>

void fork_task(void)
{
    int i;
    int print_location = 9;
    int child_number = 0;
    int allow_fork = 1;
    for (i = 0;; i++)
    {
        sys_move_cursor(1, print_location);
        if (!child_number)
            printf("> [TASK] This task is to test fork. This is father process. (%d)\n", i); 
        else
            printf("> [TASK] This task is to test fork. This is child %d  process. (%d)\n", child_number, i); 
        
        if (!allow_fork) continue;
        char c = sys_read();
        if (c < '0' || c > '4') continue;
        /* fork */
        if (sys_fork()) {
            /* father process */
            allow_fork = 0;
            continue;
        }
        /* child process */
        print_location++;
        child_number++;
        sys_priority(c - '0');
        //sys_yield();
    }
}
