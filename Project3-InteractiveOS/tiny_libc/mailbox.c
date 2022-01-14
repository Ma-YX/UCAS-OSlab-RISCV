#include <mailbox.h>
#include <string.h>
#include <sys/syscall.h>

mailbox_t mbox_open(char *name)
{
    // TODO:
    mailbox_t mailbox;
    mailbox.id = sys_mbox_open(name);
    return mailbox;
}

void mbox_close(int handle)
{
    // TODO:
    sys_mbox_close(handle);
}

int mbox_send(int handle, void *msg, int msg_length)
{
    // TODO:
    return sys_mbox_send(handle, msg, msg_length);
}

int mbox_recv(int handle, void *msg, int msg_length)
{
    // TODO:
    return sys_mbox_recv(handle, msg, msg_length);
}
