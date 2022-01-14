#include <os/mm.h>
#include <pgtable.h>

uintptr_t alloc_page_checker_build(uintptr_t va, uintptr_t pgdir);
uintptr_t alloc_page_checker_exist(uintptr_t va, uintptr_t pgdir);

ptr_t memCurr = FREEMEM;


ptr_t allocPage(pcb_t *pcb)
{
    ptr_t ret;
    if(stack_index > 0)
    ret = stack_table[--stack_index];
    else{
    // align PAGE_SIZE
    ret = ROUND(memCurr, PAGE_SIZE);
    memCurr = ret + 1 * PAGE_SIZE;
    }
    if(pcb != NULL){
        pcb->page_table[pcb->page_num++] = ret;
    }
    return ret;
}

void* kmalloc(size_t size)
{
    ptr_t ret = ROUND(memCurr, 4);
    memCurr = ret + size;
    return (void*)ret;
}

//use shm_page_table(array) to manage
uintptr_t shm_page_get(int key)
{
    // TODO(c-core):
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    int index = key % NUM_MAX_SHM;
    if(shm_page_table[index].valid == 0){
        uintptr_t pa = allocPage(NULL) - KPA_OFFSET;
        shm_page_table[index].pa = pa;
        current_running->father->shm_page_num++;
        uintptr_t vaddr = USER_STACK_ADDR - (current_running->father->shm_page_num + 1) * PAGE_SIZE;
        set_page_helper(vaddr, current_running->father->pgdir, current_running->father, pa);
        flush_all();
        clear_pgdir(pa + KPA_OFFSET);
        shm_page_table[index].num++;
        shm_page_table[index].valid = 1;
        return vaddr;
    }
    else{
        current_running->father->shm_page_num++;
        uintptr_t vaddr = USER_STACK_ADDR - (current_running->father->shm_page_num + 1) * PAGE_SIZE;
        uintptr_t pa = shm_page_table[index].pa;
        set_page_helper(vaddr, current_running->father->pgdir, current_running->father, pa);
        flush_all();
        shm_page_table[index].num++;
        return vaddr;
    }
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
    current_running = (get_current_cpu_id() == 0) ? current_running_m : current_running_s;
    PTE *pte = alloc_page_checker(addr, current_running->father->pgdir, 1);
    uintptr_t pa = ((*pte) >> 10) << 12;
    int i;
    for(i = 0; i < NUM_MAX_SHM; i++){
        if(shm_page_table[i].pa != pa){
            break;
        }
    }
    if(i != NUM_MAX_SHM){
        shm_page_table[i].num--;
    }
    else{
        return 0;
    }
    
    *pte = 0;
    flush_all();
    if(shm_page_table[i].num == 0){
        shm_page_table[i].valid = 0;
        stack_table[stack_index++] = pa + KPA_OFFSET;
    }
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
    kmemcpy((char *)dest_pgdir, (char *)src_pgdir, PAGE_SIZE);
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, pcb_t *pcb)
{
    // TODO:
    uint64_t address = pcb;
    uint64_t vpn2 = (va << 25) >> 55;
    uint64_t vpn1 = (va << 34) >> 55;
    uint64_t vpn0 = (va << 43) >> 55;
    uint64_t second_pgdir;
    uint64_t third_pgdir;
    uint64_t pgdir_entry = *((PTE *)pgdir + vpn2);
    uint64_t pgdir2_entry;
    uint64_t pgdir3_entry;
    uint64_t pgdir1_ppn;
    uint64_t pgdir2_ppn;
    uint64_t pgdir3_ppn;
    uint64_t mask_12 =_PAGE_PRESENT | _PAGE_USER ;
    uint64_t mask_3 =_PAGE_PRESENT |_PAGE_USER | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC;
    uint64_t valid1, valid2, valid3;
    valid1 = pgdir_entry & 0x1;
    if (valid1 == 0){
        second_pgdir = allocPage(pcb);
        pgdir1_ppn = (kva2pa(second_pgdir) >> 12) << 10;
        set_attribute((PTE *)pgdir + vpn2, pgdir1_ppn | mask_12);
    }
    else{
        second_pgdir = pa2kva(get_pa(pgdir_entry)); 
    }
    pgdir2_entry = *((PTE *)second_pgdir + vpn1);
    valid2 = pgdir2_entry & 0x1;
    if (valid2 == 0){
        third_pgdir = allocPage(pcb);
        pgdir2_ppn = (kva2pa(third_pgdir) >> 12) << 10;
        set_attribute((PTE *)second_pgdir + vpn1, pgdir2_ppn | mask_12);
    }
    else{
        third_pgdir = pa2kva(get_pa(pgdir2_entry));
    }
    pgdir3_entry = *((PTE *)third_pgdir + vpn0);
    valid3 = pgdir3_entry & 0x1;
    if (valid3 == 0){
        uint64_t vpa = allocPage(pcb);
        pgdir3_ppn = (kva2pa(vpa) >> 12) << 10;
        set_attribute((PTE *)third_pgdir + vpn0, pgdir3_ppn | mask_3);
        return vpa;
    }
    return 0;
}

//according to different conditions, checker needs to return different values
uintptr_t alloc_page_checker(uintptr_t va, uintptr_t pgdir, int mode){
    if(mode == 1){
        return alloc_page_checker_build(va, pgdir);
    }
    else if(mode == 2){
        return alloc_page_checker_exist(va, pgdir);
    }
    else{
        return -1;
    }
}

uintptr_t alloc_page_checker_build(uintptr_t va, uintptr_t pgdir){
    uint64_t vpn2 = (va << 25) >> 55;
    uint64_t vpn1 = (va << 34) >> 55;
    uint64_t vpn0 = (va << 43) >> 55;
    uint64_t second_pgdir;
    uint64_t third_pgdir;
    uint64_t pgdir_entry = *((PTE *)pgdir + vpn2);
    uint64_t pgdir2_entry;
    uint64_t pgdir3_entry;
    uint64_t valid1, valid2, valid3;
    valid1 = pgdir_entry & 0x1;
    if (valid1 == 0){
        return 0;
    }
    else{
        second_pgdir = pa2kva(get_pa(pgdir_entry)); 
    }
    pgdir2_entry = *((PTE *)second_pgdir + vpn1);
    valid2 = pgdir2_entry & 0x1;
    if (valid2 == 0){
        return 0;
    }
    else{
        third_pgdir = pa2kva(get_pa(pgdir2_entry));
    }
    pgdir3_entry = *((PTE *)third_pgdir + vpn0);
    return ((PTE *)third_pgdir + vpn0);
}

uintptr_t alloc_page_checker_exist(uintptr_t va, uintptr_t pgdir){
    uint64_t vpn2 = (va << 25) >> 55;
    uint64_t vpn1 = (va << 34) >> 55;
    uint64_t vpn0 = (va << 43) >> 55;
    uint64_t second_pgdir;
    uint64_t third_pgdir;
    uint64_t pgdir_entry = *((PTE *)pgdir + vpn2);
    uint64_t pgdir2_entry;
    uint64_t pgdir3_entry;
    uint64_t valid1, valid2, valid3;
    valid1 = pgdir_entry & 0x1;
    if (valid1 == 0){
        return 0;
    }
    else{
        second_pgdir = pa2kva(get_pa(pgdir_entry)); 
    }
    pgdir2_entry = *((PTE *)second_pgdir + vpn1);
    valid2 = pgdir2_entry & 0x1;
    if (valid2 == 0){
        return 0;
    }
    else{
        third_pgdir = pa2kva(get_pa(pgdir2_entry));;
    }
    pgdir3_entry = *((PTE *)third_pgdir + vpn0);
    valid3 = pgdir3_entry & 0x1;
    if (valid3 == 0){
        return 0;
    }
    else{
        return ((PTE *)third_pgdir + vpn0);
    }
}

uintptr_t set_page_helper(uintptr_t va, uintptr_t pgdir, pcb_t *pcb, uintptr_t pa)
{
    uint64_t address = pcb;
    uint64_t vpn2 = (va << 25) >> 55;
    uint64_t vpn1 = (va << 34) >> 55;
    uint64_t vpn0 = (va << 43) >> 55;
    uint64_t second_pgdir;
    uint64_t third_pgdir;
    uint64_t pgdir_entry = *((PTE *)pgdir + vpn2);
    uint64_t pgdir2_entry;
    uint64_t pgdir3_entry;
    uint64_t pgdir1_ppn;
    uint64_t pgdir2_ppn;
    uint64_t pgdir3_ppn;
    uint64_t mask_12 =_PAGE_PRESENT | _PAGE_USER;
    uint64_t mask_3 =_PAGE_PRESENT |_PAGE_USER | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC ;
    uint64_t valid1, valid2, valid3;
    valid1 = pgdir_entry & 0x1;
    if (valid1 == 0){
        second_pgdir = allocPage(pcb);
        pgdir1_ppn = (kva2pa(second_pgdir) >> 12) << 10;
        set_attribute((PTE *)pgdir + vpn2, pgdir1_ppn | mask_12);
    }
    else{
        second_pgdir = pa2kva(get_pa(pgdir_entry)); 
    }
    pgdir2_entry = *((PTE *)second_pgdir + vpn1);
    valid2 = pgdir2_entry & 0x1;
    if (valid2 == 0){
        third_pgdir = allocPage(pcb);
        pgdir2_ppn = (kva2pa(third_pgdir) >> 12) << 10;
        set_attribute((PTE *)second_pgdir + vpn1, pgdir2_ppn | mask_12);
    }
    else{
        third_pgdir = pa2kva(get_pa(pgdir2_entry));;
    }
    pgdir3_entry = *((PTE *)third_pgdir + vpn0);
    valid3 = pgdir3_entry & 0x1;
    if (valid3 == 0){
        pgdir3_ppn = (pa >> 12) << 10;
        set_attribute((PTE *)third_pgdir + vpn0, pgdir3_ppn | mask_3);
        return pa + KPA_OFFSET;
    }
    return 0;
}
