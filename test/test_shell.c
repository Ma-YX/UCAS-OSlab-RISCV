/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};
    
struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};
struct task_info task_test_multi_mbox = {(uintptr_t)&test_multi_mbox, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task, &strgenerator_task,
                                           &task_test_affinity,
                                           &task_test_multi_mbox};
static int num_test_tasks = 8;

#define SHELL_BEGIN 25
#define CMD_NUM 7

typedef void (*function)(void *parameter);

pid_t shell_exec(char *task_id);
int shell_waitpid(char *pid);
int shell_kill(char *pid);
void shell_exit(void);
void shell_clear(void);
void shell_ps(void);
void shell_taskset(char *arg1, char *arg2, char *arg3);

char read_uart(void);
void parse_cmd(char *cmd);
int match_cmd(char *para);

struct{
    char *name;
    function func;
    int para_num; 
} cmd_table[] = {
    {"exec"   , &shell_exec   , 1},
    {"wait"   , &shell_waitpid, 1},
    {"kill"   , &shell_kill   , 1},
    {"exit"   , &shell_exit   , 0},
    {"clear"  , &shell_clear  , 0},
    {"ps"     , &shell_ps     , 0},
    {"taskset", &shell_taskset, 2}
};

void shell_clear(void){
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
}

void shell_ps(void){
    sys_process_show();
}

int my_atoi(char *c){
    int tid = 0;
    int i = 0;
    while(c[i]){
        tid = c[i] - '0' + tid * 10;
        i++;
    }
    return tid;
}

pid_t shell_exec(char *task_id){
    int tid = my_atoi(task_id);
    int pid;
    if(tid > num_test_tasks){
        printf("ERROR: unknown task");
        return -1;
    }
    else{
        pid = sys_spawn(test_tasks[tid], NULL, AUTO_CLEANUP_ON_EXIT);
        printf("exec process[%d]\n", pid);
        return tid;
    }
}

int shell_waitpid(char *pid){
    pid_t wait_pid = (pid_t)my_atoi(pid);
    int waiting;
    waiting = sys_waitpid(wait_pid);
    if(waiting){
        printf("waiting for process[%d]\n", wait_pid);
    }
    else{
        printf("ERROR: process[%d] not found.\n", wait_pid);
    }
}

int shell_kill(char *pid){
    pid_t kill_pid = (pid_t)my_atoi(pid);
    int killed;
    killed = sys_kill(kill_pid);
    if(killed){
        printf("kill process[%d]\n", kill_pid);
    }
    else{
        printf("ERROR: process[%d] not found.\n", kill_pid);
    }
}

void shell_exit(){
    sys_exit();
}

void shell_taskset(char *arg1, char *arg2, char *arg3){
    int num;
    int mask;
    pid_t set_pid;
    if(arg1[0] == '-' && arg1[1] == 'p'){
        mask = my_atoi(arg2);
        set_pid = my_atoi(arg3);
        sys_taskset_p(mask, set_pid);
    }
    else{
        mask = my_atoi(arg1);
        num = my_atoi(arg2);
        sys_taskset_spawn(mask, test_tasks[num], AUTO_CLEANUP_ON_EXIT);
    }
}

char read_uart(void){
    char c;
    int tmp;
    while((tmp = sys_get_char()) == -1) ;
    c = (char)tmp;
    return c;
}

int match_cmd(char *para){
    int i;
    for(i = 0; i < CMD_NUM; i++){
        if(strcmp(para, cmd_table[i].name) == 0){
            return i;
        }
    }
    return -1;
}

void parse_cmd(char *cmd){
    char arg[10][10] = {0};
    int cmd_id;
    int i = 0;
    char *parse = cmd;
    int j = 0, k = 0;
    while(parse[j] != '\0'){
        if(parse[j] != ' ')
            arg[i][k++] = parse[j];
        else{
            arg[i][k] = '\0';
            i++;
            k = 0;
        }
        j++;
    }
    cmd_id = match_cmd(arg[0]);
    //taskset
    if(cmd_id == 6){
        if(i == 3){
            i = 2;
        }
    }
    if(cmd_id < 0){
        printf("ERROR: unknown command %s\n", arg[0]);
        printf("> root@UCAS_OS: ");
    }
    else if(cmd_table[cmd_id].para_num != i){
        printf("ERROR: unmatched parameters %s\n", arg[0]);
        printf("> root@UCAS_OS: ");
    }
    else{
        /*
        void *parameter;
        if(cmd_table[cmd_id].para_num == 0){
            parameter = NULL;
        }
        else{
            parameter = arg[1];
        }
        */
        if(cmd_id != 6){
            cmd_table[cmd_id].func(arg[1]);
        }
        else{
            shell_taskset(arg[1], arg[2], arg[3]);
        }
        printf("> root@UCAS_OS: ");
    }
}

void test_shell()
{
    // TODO:
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- COMMAND -------------------\n");
    printf("> root@UCAS_OS: ");
    char buffer[20] = {0};
    int i = 0;
    while (1)
    {
        // TODO: call syscall to read UART port
        char c = read_uart();
        if(c == '\r'){
        // TODO: parse input
        // TODO: ps, exec, kill, clear
            printf("\n");
            buffer[i] = '\0';
            parse_cmd(buffer);
            i = 0;
        }
        // note: backspace maybe 8('\b') or 127(delete)
        else if(c == 8 || c == 127){
            buffer[i--] = 0;
            sys_serial_write(c);
        }
        else{
            buffer[i++] = c;
            sys_serial_write(c);
        }
    }
}
