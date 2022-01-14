#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>
#include <os/smp.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    uintptr_t kva = io_base;
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    printk("phys_addr: %d, size: %d\n\r", phys_addr, size);
    while(size){
        set_page_helper_k(io_base, current_running->pgdir, current_running, phys_addr);
        size -= PAGE_SIZE;
        io_base += PAGE_SIZE;
        phys_addr += PAGE_SIZE;
        //printk("phys_addr: %d, size: %d\n\r", phys_addr, size);
    }
    flush_all();
    return kva;
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
