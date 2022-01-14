  #include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <os/sched.h>
#include <type.h>

LIST_HEAD(timers);
LIST_HEAD(sleep_queue);

uint64_t time_elapsed = 0;
uint32_t time_base = 0;

pcb_t *entry_sleep_queue(list_head* queue)
{
    pcb_t *tmp = list_entry(queue, pcb_t, list);
    return tmp;
}

void timer_creat(TimerCallback func, void *parameter, uint64_t tick){
    disable_preempt();
    current_running->timer.timeout_tick = tick + get_ticks();
    current_running->timer.callback_func = func;
    current_running->timer.parameter = parameter;
    add_queue(&sleep_queue, current_running);
    enable_preempt();
}

int time_checking(void){
    disable_preempt();
    int flag = 0;
    uint64_t ticks;
    pcb_t * tmp;
    list_node_t * sleep_node;
    sleep_node = sleep_queue.next;
    while(!list_empty(&sleep_queue) && sleep_node != &sleep_queue){
        tmp = entry_sleep_queue(sleep_node);
        if((ticks = get_ticks()) >= tmp->timer.timeout_tick){
            list_del(sleep_node);
            tmp->timer.callback_func(tmp->timer.parameter);
            flag = 1;
        }
        sleep_node = sleep_node->next;
    }
    enable_preempt();
    return flag;
}

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}
