#define PCI_VID_DID 0x00
#define PCI_STA_CMD 0x04
#define PCI_CLASS 0x08
#define PCI_BAR0 0x10
#define PCI_BAR1 0x14
#define PCI_BAR2 0x18
#define PCI_BAR3 0x1C
#define PCI_BAR4 0x20
#define PCI_BAR5 0x24

struct pci_dev {
    uint8 bus, device, function;
    uint16 vendor_id, device_id;
    uint8 class_code;
    uint32 bar0;
};

uint32 pci_config_read(uint, uint, uint, uint);
void pci_config_write(uint, uint, uint, uint, uint32);
