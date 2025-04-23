#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "pci.h"
#include "e1000.h"

#define NPCI_CLASSES 7
static const char *pci_class[] =
{
  [0x0] = "Unknown",
  [0x1] = "Storage controller",
  [0x2] = "Network controller",
  [0x3] = "Display controller",
  [0x4] = "Multimedia device",
  [0x5] = "Memory controller",
  [0x6] = "Bridge device",
};

struct pci_dev_info {
  uint16 vendor_id;
  uint16 device_id;
  void (*init)(struct pci_dev *);
} dev_info[] = {
  {0x8086, 0x100E, &e1000init},
};

struct pci_dev pci_devices[NPCIDEV];
uint num_pci_devs = 0;

uint32
pci_config_read(uint bus, uint device, uint function, uint offset)
{
  uint32 off;
  off = (bus << 20) | (device << 15) | (function << 12) | offset;
  return mem_read32((volatile void *) (PCIE_ECAM_BASE + off));
}

void
pci_config_write(uint bus, uint device, uint function, uint offset, uint32 value)
{
  uint32 off;
  off = (bus << 20) | (device << 15) | (function << 12) | offset;
  mem_write32((volatile void *) (PCIE_ECAM_BASE + off), value);
}

static void
list_dev()
{
  uint i;
  for (i = 0; i < num_pci_devs; i++) {
    printf("PCI: Bus %d Device %d Function %d Vendor ID 0x%x Device ID 0x%x Class Code 0x%x (%s)\n",
           pci_devices[i].bus,
           pci_devices[i].device,
           pci_devices[i].function,
           pci_devices[i].vendor_id,
           pci_devices[i].device_id,
           pci_devices[i].class_code,
           (pci_devices[i].class_code < NPCI_CLASSES) ? pci_class[pci_devices[i].class_code] : "Unknown");
  }
}

static uint32
get_viddid(uint bus, uint device, uint function)
{
  uint32 conf;
  conf = pci_config_read(bus, device, function, PCI_VID_DID);
  return conf;
}

static uint8
get_classcode(uint bus, uint device, uint function)
{
  uint32 conf;
  conf = pci_config_read(bus, device, function, PCI_CLASS);
  return (conf >> 24) & 0xFF;
}

static uint32
get_bar0(uint bus, uint device, uint function)
{
  uint32 conf;
  conf = pci_config_read(bus, device, function, PCI_BAR0);
  return conf;
}

static struct pci_dev_info *lookup_dev_info(uint vendor_id, uint device_id)
{
  uint i;
  for (i = 0; i < sizeof(dev_info) / sizeof(struct pci_dev_info); i++) {
    if (dev_info[i].vendor_id == vendor_id && dev_info[i].device_id == device_id) {
      return &dev_info[i];
    }
  }
  return 0;
}

void
scan_bus(uint bus)
{
  struct pci_dev *dev;
  struct pci_dev_info *info;
  uint8 device, function;
  uint16 vendor_id, device_id;
  uint32 conf;

  for (device = 0; device < 32; device++) {
    for (function = 0; function < 8; function++) {
      conf = get_viddid(bus, device, function);
      vendor_id = conf & 0xFFFF;
      device_id = (conf >> 16) & 0xFFFF;
      if (vendor_id == 0xFFFF) continue;

      dev = &pci_devices[num_pci_devs];
      dev->bus = bus;
      dev->device = device;
      dev->function = function;
      dev->vendor_id = vendor_id;
      dev->device_id = device_id;
      dev->class_code = get_classcode(bus, device, function);
      dev->bar0 = get_bar0(bus, device, function);
      
      info = lookup_dev_info(vendor_id, device_id);
      if (info != 0) info->init(dev);

      num_pci_devs++;

      if (NPCIDEV <= num_pci_devs) {
        printf("PCI: Too many devices\n");
        return;
      }
    }
  }
}

void
pciinit(void)
{
  scan_bus(0);
  list_dev();
}