/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <os/syscall_number.h>
#include <stdint.h>
#include <os/sched.h>
 
#define SCREEN_HEIGHT 80


extern long invoke_syscall(long, long, long, long, long);

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
void sys_show_exec();

void sys_sleep(uint32_t);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

int sys_mutex_get(int);
int sys_mutex_lock(int);
int sys_mutex_unlock(int);

long sys_get_timebase();
long sys_get_tick();

void sys_priority(uint32_t priority);
char sys_read();
void sys_yield();

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode);
void sys_exit(void);
int sys_kill(pid_t pid);
int sys_waitpid(pid_t pid);
void sys_process_show(void);
void sys_screen_clear(void);
pid_t sys_getpid();
int sys_get_char(void);

void sys_serial_write(char c);
#define BINSEM_OP_LOCK 0 // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

int binsemget(int key);
int binsemop(int binsem_id, int op);
//synchronization primitives
//semaphore
int sys_semaphore_init(int val);
int sys_semaphore_up(int handle);
int sys_semaphore_down(int handle);
int sys_semaphore_destory(int handle);
//barrier
int sys_barrier_init(int count);
int sys_barrier_wait(int handle);
int sys_barrier_destory(int handle);
//mailbox
int sys_mbox_open(char *name);
void sys_mbox_close(int handle);
int sys_mbox_send(int handle, void *msg, int msg_length);
int sys_mbox_recv(int handle, void *msg, int msg_length);
int sys_mbox_send_recv(int send, int recv, void *msg);

void sys_taskset_p(int mask, pid_t pid);
void sys_taskset_spawn(int mask, task_info_t *info, spawn_mode_t mode);
long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength);
void sys_net_send(uintptr_t addr, size_t length);
void sys_net_irq_mode(int mode);


void *shmpageget(int key);
void shmpagedt(void *addr);

void sys_mkfs();
void sys_statfs();
void sys_cd(char *name);
void sys_mkdir(char *name);
void sys_rmdir(char *name);
void sys_fs_ls(char *name);
int sys_touch(char *name);
int sys_cat(char*name);
int sys_fopen(char *name,int access);
int sys_close(int fd);
int sys_fread(int fd,char *buff,int size);
int sys_fwrite(int fd,char *buff,int size);
int sys_rmfile(char *name);
int sys_ln(char*src,char*dst);
int sys_ls_l(char *name);
int sys_lseek(int fd,int offset,int whence);
#endif
