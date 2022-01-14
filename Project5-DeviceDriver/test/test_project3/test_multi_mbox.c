#include <time.h>
#include <test_project3/test3.h>
#include <mthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <mailbox.h>
#include <sys/syscall.h>

char *mailbox_name[3] = {"mailbox0", "mailbox1", "mailbox2"};
void mailbox(int);
extern void genrerateRandomString(char *, int);

void test_multi_mbox(void)
{
    struct task_info task = {(uintptr_t)&mailbox, USER_PROCESS};
    int i;
    for (i = 0; i < 3; i++) {
        sys_spawn(&task, i, AUTO_CLEANUP_ON_EXIT);
        sys_sleep(1);
    }
    sys_exit();
}

void mailbox(int id)
{
    mailbox_t my_mbox = mbox_open(mailbox_name[id]);
    int position = id + 22;

    char buf[MAX_MBOX_LENGTH];
    int send_sum = 0, recv_sum = 0;
    mailbox_t send_to[2];
    send_to[0] = mbox_open(mailbox_name[id ? 0 : 1]);
    send_to[1] = mbox_open(mailbox_name[id == 2 ? 1 : 2]);
    for (;;)
    {
        int len = rand() % MAX_MBOX_LENGTH + 1;
        generateRandomString(buf, len);
        int flag = sys_mbox_send_recv(send_to[(rand()&(1024))>>10].id, my_mbox.id, buf);
        if (flag & 0x1) send_sum += flag >> 2;
        if (flag & 0x2) recv_sum += flag >> 2;
        sys_move_cursor(1, position);
        printf("[Mailbox %d]: total sent : %d  total received : %d\n", id, send_sum, recv_sum);
    }
}
