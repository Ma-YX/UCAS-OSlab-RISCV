C-core(except task 4):
executable programes: fly, rw, lock, swap, mailbox, consensus, threadadd(it will launch processadd), send, recv
new programmes: send, recv
    usage: send: exec send 0(for polling)/1(for irq); exec recv number 0(for polling)/1(for irq)






When use double cores, prepare two user processes ready(in the other words, there are potential bugs when switch to pid0)
Qemu seems not to support swap while running double cores; may be blocks are different.