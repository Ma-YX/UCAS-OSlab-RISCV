#ifndef SHM_H
#define SHM_H
extern void *shmpageget(int key);

extern void shmpagedt(void *addr);

#endif /* SHM_H */
