#include <os/syscall.h>
#include <os/stdio.h>
long (*syscall[NUM_SYSCALLS])();

void handle_syscall(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    // syscall[fn](arg1, arg2, arg3)
    regs->sepc = regs->sepc + 4;
    regs->regs[10] = syscall[regs->regs[17]](regs->regs[10],
                                              regs->regs[11],
                                              regs->regs[12],
                                              regs->regs[13]);
    //if(regs->regs[17] == SYSCALL_NET_SEND){
    //    prints("exit send syscall, sepc: 0x%x\n\r", regs->sepc);
    //}
}
