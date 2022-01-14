#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
#define NUM_ADDRESS 16

#define BUFFER_LENGTH (4*(MAX_MBOX_LENGTH))         
int main(int argc, char* argv[]){
    char *prog_name = argv[0];
    int print_location = 1;
    if (argc > 1) {
        print_location = atol(argv[1]);
    }
    sys_move_cursor(1, print_location);

    uint64_t* address_table[NUM_ADDRESS];
    int i;
    for(i = 0; i < NUM_ADDRESS; i++){
        address_table[i] = 0xff000000 + i * 0x1000;
        *address_table[i] = (uint64_t)address_table[i];
    }
    int success = 1;

    for(i = 0; i < NUM_ADDRESS; i++){
        if(*address_table[i] == (uint64_t)address_table[i])
        printf("[%d]address:%u match success!\n", i, address_table[i]);
        else{
            printf("[%d]address:%u match fail!\n", i, address_table[i]);
            success = 0;
            break;
        }
    }

    if(success == 1)
    printf("---PASS SWAP TEST: %dMATCHED---\n", NUM_ADDRESS);
    else
    printf("---ERROR!SOME ADDRESSES NOT MATCH---\n");
    //sys_exit();
}
