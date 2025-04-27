#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "pci.h"
#include "e1000_dev.h"

#define RX_DESC_NUM 16 // to be a multiple of 8
#define PACKET_SIZE 2048

static volatile uint32 *regs;
static uint8 rxbuf[RX_DESC_NUM][PACKET_SIZE];
static struct rx_desc rx_descs[RX_DESC_NUM] __attribute__((aligned(16)));

struct spinlock e1000_lock;

void
e1000init(struct pci_dev *dev)
{
  uint i;
  uint32 status;

  initlock(&e1000_lock, "e1000");
  pci_func_enable(dev);
  regs = dev->regs;

  regs[E1000_IMS] = 0; // disable interrupts
  regs[E1000_CTL] |= E1000_CTL_RST; // reset
  regs[E1000_IMS] = 0; // redisable interrupts
  __sync_synchronize();

  regs[E1000_CTL] |= E1000_CTL_FD | E1000_CTL_ASDE | E1000_CTL_SLU;
  status = regs[E1000_STATUS] & 0x0FFF;
  printf("E1000: Device Status: 0x%x\n", status);

  // setup receive descriptor
  regs[E1000_RCTL] &= ~E1000_RCTL_EN; // disable rx
  for (i = 0; i < RX_DESC_NUM; i++) {
    rx_descs[i].addr = (uint64)&rxbuf[i];
    rx_descs[i].status = 0;
    rx_descs[i].errors = 0;
  }
  if (sizeof(rx_descs) % 128 != 0) panic("e1000 error");
  regs[E1000_RDBAL] = (uint64) rx_descs;
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_DESC_NUM - 1;
  regs[E1000_RDLEN] = sizeof(rx_descs);

  // mac address
  regs[E1000_RA] = 0x12005452;
  regs[E1000_RA+1] = 0x5634 | (1<<31);

  for (int i = 0; i < 4096/32; i++) regs[E1000_MTA + i] = 0;

  regs[E1000_RCTL] = E1000_RCTL_EN |
    E1000_RCTL_BAM |
    E1000_RCTL_SZ_2048 |
    E1000_RCTL_SECRC;
  
  regs[E1000_RDTR] = 0;
  regs[E1000_RADV] = 0;
  regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
  __sync_synchronize();

  while (1) {
    for (i = 0; i < RX_DESC_NUM; i++) {
      if (rx_descs[i].status & E1000_RXD_STAT_DD) {
        printf("E1000: Received packet %d\n", i);
        rx_descs[i].status &= ~E1000_RXD_STAT_DD;
        for (int j = 0; j < rx_descs[i].length; j++) {
          printf("%x ", rxbuf[i][j]);
        }
        printf("\n");
      }
    }
  }
}

// static void
// e1000_recv(void)
// {
// }
