#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "pci.h"
#include "e1000.h"

void
e1000init(struct pci_dev *dev)
{
    pci_config_write(dev->bus, dev->device, dev->function, PCI_STA_CMD, 0x0007); // enable bus master and memory access

    pci_config_write(dev->bus, dev->device, dev->function, PCI_BAR0, 0xFFFFFFFF);
    uint32 bar0_size_raw = pci_config_read(dev->bus, dev->device, dev->function, PCI_BAR0);
    uint32 bar0_size = ~(bar0_size_raw & ~0xF) + 1;
    printf("E1000: BAR0 size = 0x%x\n", bar0_size);
    pci_config_write(dev->bus, dev->device, dev->function, PCI_BAR0, PCIE_MMIO_BASE);
    uint32 bar0 = pci_config_read(dev->bus, dev->device, dev->function, PCI_BAR0);
    uint32 addr = bar0 & ~0xF; // clear the lower bits
    printf("E1000: BAR0: 0x%x\n", addr);
    uint32 status = mem_read32((volatile void *) (addr + 0x08L));
    status &= 0x0FFF;
    printf("E1000: Status: 0x%x\n", status);
}