#include <asm.h>
#include <csr.h>

ENTRY(setup_exception)
  /* TODO: 
   * save exception_handler_entry into STVEC
   * enable global exceptions */
  la t0, exception_handler_entry
  csrw stvec, t0
  /*
  li s0, SR_SIE
  csrw sstatus, s0
  */
  li t0, SIE_STIE//SIE_SSIE | SIE_STIE | SIE_SEIE
  csrw sie, t0
  jr ra
ENDPROC(setup_exception)

ENTRY(setup_Umode)
  csrw sepc, ra
  li s0, SR_SPP
  csrw sstatus, s0
  sret
ENDPROC(setup_Umode)
