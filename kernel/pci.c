#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"

uint32 pci_config_read(uint bus, uint device, uint function, uint offset) {
  uint32 off = (bus << 20) | (device << 15) | (function << 12) | offset;
  return mem_read32((volatile void *) (PCIE_ECAM_BASE + off));
}

void pciinit(void) {
  printf("PCI initialization\n");
  int bus = 0;
  for (int device = 0; device < 32; device++) {
    for (int function = 0; function < 8; function++) {
      uint32 data = pci_config_read(bus, device, function, 0);
      uint16 vendor_id = data & 0xFFFF;
      uint16 device_id = (data >> 16) & 0xFFFF;
      if (vendor_id != 0xFFFF) {
        printf("Found device: bus %d, device %d, function %d, vendor ID: 0x%x, device ID: 0x%x\n",
                bus, device, function, vendor_id, device_id);
      }
    }
  }
}