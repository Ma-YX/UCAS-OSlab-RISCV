#include <stdio.h>
#include <sys/syscall.h>

void prior_task1()
{
    int i;
    int print_location = 7;
    sys_priority(1);
    for (i = 0;; i++)
    {
        sys_move_cursor(1, print_location);
        printf("> [TASK] This task is to test priority. Task's priority is 1. (%d)\n", i);
    }
}

void prior_task2()
{
    int i;
    int print_location = 8;
    sys_priority(2);
    for (i = 0;; i++)
    {
        sys_move_cursor(1, print_location);
        printf("> [TASK] This task is to test priority. Task's priority is 2. (%d)\n", i);
    }
}
