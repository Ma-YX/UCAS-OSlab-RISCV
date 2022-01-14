#ifndef SHM_H
#define SHM_H
void *shmpageget(int key){
    return invoke_syscall(SYSCALL_SHM_PAGE_GET, key, IGNORE, IGNORE, IGNORE);
}

void shmpagedt(void *addr){
    return invoke_syscall(SYSCALL_SHM_PAGE_DT, addr, IGNORE, IGNORE, IGNORE);
}
#endif /* SHM_H */
