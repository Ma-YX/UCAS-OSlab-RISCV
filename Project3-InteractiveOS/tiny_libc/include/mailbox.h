#ifndef INCLUDE_MAIL_BOX_
#define INCLUDE_MAIL_BOX_

#define MAX_MBOX_LENGTH (64)

// TODO: please define mailbox_t;
// mailbox_t is just an id of kernel's mail box.
typedef struct mailbox
{
    int id; 
} mailbox_t;

mailbox_t mbox_open(char *name);
void mbox_close(int handle);
int mbox_send(int handle, void *msg, int msg_length);
int mbox_recv(int handle, void *msg, int msg_length);

#endif
