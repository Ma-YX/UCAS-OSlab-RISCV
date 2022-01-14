#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>
#include <os/smp.h>

#define MAX_NUM_TURN 32

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode;

volatile int rx_curr = 0, rx_tail = 0;

long do_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    // TODO: 
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?
    //return ret_t;
    long status;
    int recv_packet = 0;
    while(num_packet > recv_packet){
        prints("recv_packet: %d\n", recv_packet);
        if((num_packet - recv_packet) < MAX_NUM_TURN){
            int num = num_packet - recv_packet;
            status = EmacPsRecv(&EmacPsInstance, kva2pa(&rx_buffers), num);  
            status = EmacPsWaitRecv(&EmacPsInstance, num, rx_len); 
            //copy buffer
            int i;
            for(i = 0; i < num; i++){
                kmemcpy(addr, &rx_buffers[i], rx_len[i]);
                addr += rx_len[i];
                frLength[i + recv_packet] = rx_len[i];
            }
            recv_packet += num;
        }
        else{
            int num = MAX_NUM_TURN;
            status = EmacPsRecv(&EmacPsInstance, kva2pa(&rx_buffers), num);  
            status = EmacPsWaitRecv(&EmacPsInstance, num, rx_len); 
            //copy buffer
            int i;
            for(i = 0; i < num; i++){
                kmemcpy(addr, &rx_buffers[i], rx_len[i]);
                addr += rx_len[i];
                frLength[i + recv_packet] = rx_len[i];
            }
            recv_packet += num;
        }
    }
    return status;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // TODO:
    long status;
    // send all packet
    //copy buffer
    kmemcpy(&tx_buffer, addr, length);
    status = EmacPsSend(&EmacPsInstance, kva2pa(&tx_buffer), length); 

    status = EmacPsWaitSend(&EmacPsInstance); 
    //prints("do_net_send end\n\r");
    // maybe you need to call drivers' send function multiple times ?

}

void do_net_irq_mode(int mode)
{
    // TODO:
    // turn on/off network driver's interrupt mode
    //prints("mode: %d\n", mode);
    set_irq_mode(&EmacPsInstance, mode);

}
