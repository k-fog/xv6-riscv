#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "pci.h"
#include "e1000_dev.h"

static volatile uint32 *regs;

struct spinlock e1000_lock;

void
e1000init(struct pci_dev *dev)
{
  initlock(&e1000_lock, "e1000");
  pci_func_enable(dev);

  regs = dev->regs;
  uint32 status = dev->regs[E1000_STATUS];
  status &= 0x0FFF;
  printf("E1000: Status: 0x%x\n", status);
}