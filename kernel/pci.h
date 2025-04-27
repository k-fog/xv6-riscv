#define PCI_VID_DID 0x00
#define PCI_CMD_STA 0x04
#define PCI_CLASS   0x08
#define PCI_BAR0    0x10
#define PCI_BAR1    0x14
#define PCI_BAR2    0x18
#define PCI_BAR3    0x1C
#define PCI_BAR4    0x20
#define PCI_BAR5    0x24
#define	PCI_COMMAND_IO_ENABLE      0x00000001
#define	PCI_COMMAND_MEM_ENABLE     0x00000002
#define	PCI_COMMAND_MASTER_ENABLE  0x00000004

struct pci_dev {
    uint8 bus, device, function;
    uint16 vendor_id, device_id;
    uint8 class_code;
    volatile uint32 *regs;
};

uint32 pci_config_read(uint, uint, uint, uint);
void pci_config_write(uint, uint, uint, uint, uint32);

void pci_func_enable(struct pci_dev *);

void e1000init(struct pci_dev *dev);