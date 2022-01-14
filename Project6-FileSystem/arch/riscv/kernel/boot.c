/* RISC-V kernel boot stage */
//#include <context.h>
#include <os/elf.h>
#include <pgtable.h>
#include <sbi.h>

typedef void (*kernel_entry_t)(unsigned long, uintptr_t);

extern unsigned char _elf_main[];
extern unsigned _length_main;

/********* setup memory mapping ***********/
uintptr_t alloc_page()
{
    static uintptr_t pg_base = PGDIR_PA;
    pg_base += 0x1000;
    return pg_base;
}

// using 2MB large page
void map_page(uint64_t va, uint64_t pa, PTE *pgdir)
{
    va &= VA_MASK;
    uint64_t vpn2 =
        va >> (NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS);
    uint64_t vpn1 = (vpn2 << PPN_BITS) ^
                    (va >> (NORMAL_PAGE_SHIFT + PPN_BITS));
    if (pgdir[vpn2] == 0) {
        // alloc a new second-level page directory
        set_pfn(&pgdir[vpn2], alloc_page() >> NORMAL_PAGE_SHIFT);
        set_attribute(&pgdir[vpn2], _PAGE_PRESENT);
        clear_pgdir(get_pa(pgdir[vpn2]));
    }
    PTE *pmd = (PTE *)get_pa(pgdir[vpn2]);
    if(pmd[vpn1] == 0){
    set_pfn(&pmd[vpn1], pa >> NORMAL_PAGE_SHIFT);
    set_attribute(
        &pmd[vpn1], _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE |
                        _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
    }
}

void enable_vm()
{
    // write satp to enable paging
    set_satp(SATP_MODE_SV39, 0, PGDIR_PA >> NORMAL_PAGE_SHIFT);
    local_flush_tlb_all();
}

/* Sv-39 mode
 * 0x0000_0000_0000_0000-0x0000_003f_ffff_ffff is for user mode
 * 0xffff_ffc0_0000_0000-0xffff_ffff_ffff_ffff is for kernel mode
 */
void setup_vm(unsigned long mhartid)
{
    PTE *early_pgdir = (PTE *)PGDIR_PA;
    if(mhartid == 0){
    clear_pgdir(PGDIR_PA);
    // map kernel virtual address(kva) to kernel physical
    // address(kpa) kva = kpa + 0xffff_ffc0_0000_0000 use 2MB page,
    // map all physical memory
    for (uint64_t kva = 0xffffffc050200000lu;
         kva < 0xffffffc060000000lu; kva += 0x200000lu) {
        map_page(kva, kva2pa(kva), early_pgdir);
    }
    }
    // map boot abooddress
        map_page(0x50200000lu, 0x50200000lu, early_pgdir);
        map_page(0x50400000lu, 0x50400000lu, early_pgdir);
    enable_vm();
}

uintptr_t directmap(uintptr_t kva, uintptr_t pgdir,uintptr_t useless)
{
    // ignore pgdir
    return kva;
}

kernel_entry_t start_kernel = NULL;

/*********** start here **************/
int boot_kernel(unsigned long mhartid, uintptr_t riscv_dtb)
{
    if (mhartid == 0) {
        setup_vm(mhartid);
        start_kernel =
            (kernel_entry_t)load_elf(_elf_main, _length_main,
                                     PGDIR_PA,0, directmap);
    } else {
        setup_vm(mhartid);
    }
    start_kernel(mhartid, riscv_dtb);
    return 0;
}
