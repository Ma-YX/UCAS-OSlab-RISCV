#include <stdio.h>
#include <sys/syscall.h>

#define NUM_PAGE 1

#define PAGE_SIZE 4096
#define START_ADDR 2333333lu

int main(int argc, char* argv[])
{
    int print_location = 1;
    if (argc > 1)
        print_location = (int) atol(argv[1]);
    sys_move_cursor(1, print_location);

    int i;
    for (i = 0; i < NUM_PAGE; i++) {
        volatile long *addr = START_ADDR + i * PAGE_SIZE;
        *addr = i * 2 + 1;
    }
    //int pid = sys_fork();
    //if (pid) {
        //father process
        printf("This is Father Process.\n");
        for (i = 0; i < NUM_PAGE; i++) {
            volatile long *addr = START_ADDR + i * PAGE_SIZE;
            *addr = i * 2 - 1;
        }
        printf("Father Process Write Success!\n");
        sys_sleep(1);
        for (i = 0; i < NUM_PAGE; i++) {
            volatile long *addr = START_ADDR + i * PAGE_SIZE;
            long v = *addr;
            if (v != i * 2 - 1) {
                printf("Father Process Read Error! addr[%d] = %d\n", i, v);
                return 0;
            }
        }
        printf("Father Process Read Success!\n");
    //} else {
        //child process
        sys_move_cursor(1, print_location + 3);
        printf("This is Child Process.\n");
        sys_sleep(1);
        for (i = 0; i < NUM_PAGE; i++) {
            volatile long *addr = START_ADDR + i * PAGE_SIZE;
            long v = *addr;
            if (v != i * 2 - 1) {
                printf("Child Process Read-1 Error! addr[%d] = %d\n", i, v);
                return 0;
            }
            *addr = i * 2;
        }
        printf("Child Process Read-1 Success!\n");

        sys_sleep(1);
        for (i = 0; i < NUM_PAGE; i++) {
            volatile long *addr = START_ADDR + i * PAGE_SIZE;
            long v = *addr;
            if (v != i * 2) {
                printf("Child Process Read-2 Error! addr[%d] = %d\n", i, v);
                return 0;
            }
        }
        printf("Child Process Read-2 Success!\n");
    //}
    return 0;
}
