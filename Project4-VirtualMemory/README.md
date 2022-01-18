#C-core(except task 5):

executable programes: fly, rw, lock, swap, mailbox, consensus, threadadd(it will launch processadd)







When use double cores, prepare two user processes ready(in the other words, there are potential bugs when switch to pid0)

Qemu seems not to support swap while running double cores; may be blocks are different.
