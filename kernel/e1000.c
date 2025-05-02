#include "types.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "pci.h"
#include "e1000_dev.h"

#define RX_DESC_NUM 16 // to be a multiple of 8
#define TX_DESC_NUM 8
#define PACKET_SIZE 2048

static volatile uint32 *regs;
static uint8 rxbuf[RX_DESC_NUM][PACKET_SIZE];
static struct rx_desc rx_descs[RX_DESC_NUM] __attribute__((aligned(16)));
static struct tx_desc tx_descs[TX_DESC_NUM] __attribute__((aligned(16)));

#define E1000_IPGT 8
#define E1000_IPGR1 8
#define E1000_IPGR2 6

struct spinlock e1000_lock;

static void
e1000init_recv(void)
{
  uint i;
  uint32 status;

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
  printf("E1000: Receive initialized\n");
  
  regs[E1000_RDTR] = 0;
  regs[E1000_RADV] = 0;

  regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
  regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
  regs[E1000_IMS] = (1 << 7);
}

static void
e1000init_transmit(void)
{
  uint i;
  regs[E1000_TDBAL] = (uint64) tx_descs;
  regs[E1000_TDLEN] = sizeof(tx_descs);
  if (sizeof(tx_descs) % 128 != 0) panic("e1000 error");
  regs[E1000_TDH] = regs[E1000_TDT] = 0;

  for (i = 0; i < TX_DESC_NUM; i++) {
    tx_descs[i].status = E1000_TXD_STAT_DD;
  }

  regs[E1000_TCTL] = E1000_TCTL_EN |
    E1000_TCTL_PSP |
    (0x10 << E1000_TCTL_CT_SHIFT) |
    (0x40 << E1000_TCTL_COLD_SHIFT);
  regs[E1000_TIPG] = E1000_IPGT | (E1000_IPGR1 << 10) | (E1000_IPGR2 << 20); // inter-pkt gap
  printf("E1000: Transmit initialized\n");
}

void
e1000init(struct pci_dev *dev)
{

  initlock(&e1000_lock, "e1000");
  pci_func_enable(dev);
  regs = dev->regs;

  // reset the device
  regs[E1000_IMS] = 0; // disable interrupts
  regs[E1000_CTL] |= E1000_CTL_RST;
  regs[E1000_IMS] = 0; // redisable interrupts
  __sync_synchronize();

  e1000init_recv();
  e1000init_transmit();

  uint16 data[] = {0xBEEF, 0xCAFE, 0xDEAD, 0xBEEF};
  uint len = 4 * sizeof(uint16);
  for (int i = 0; i < 10; i++) e1000transmit(data, len);
}

// static void
// e1000recv(void)
// {
// }

char
e1000transmit(void *buf, uint len)
{
  uint8 status;
  uint32 tail;

  tail = regs[E1000_TDT];
  tx_descs[tail].addr = (uint64)buf;
  tx_descs[tail].length = len;
  tx_descs[tail].cmd = E1000_TXD_CMD_RS | E1000_TXD_CMD_EOP;
  regs[E1000_TDT] = (tail + 1) % TX_DESC_NUM;
  status = 0;
  while (!status) status = tx_descs[tail].status & 0x0F;
  if (status != E1000_TXD_STAT_DD) {
    printf("E1000: Transmit error: 0x%x\n", status);
    return -1;
  }
  printf("E1000: Transmit status: 0x%x\n", status);
  return 0;
}

void
e1000intr(void)
{
  uint32 status = regs[E1000_ICR];
  printf("E1000: Interrupt status: 0x%x\n", status);
}