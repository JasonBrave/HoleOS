OBJS = \
	core/console.o\
	proc/exec/exec.o\
	core/kalloc.o\
	core/lapic.o\
	core/main.o\
	core/mp.o\
	core/picirq.o\
	core/proc.o\
	common/sleeplock.o\
	common/spinlock.o\
	common/string.o\
	core/swtch.o\
	proc/syscall/syscall.o\
	proc/syscall/sysfile.o\
	proc/syscall/sysproc.o\
	core/trapasm.o\
	core/trap.o\
	vectors.o\
	core/vm.o\
	filesystem/initramfs/initramfs.o\
	filesystem/vfs/dir.o\
	filesystem/vfs/vfs.o\
	filesystem/vfs/filedesc.o\
	proc/exec/elf.o\
	driver/pci/pci.o\
	driver/pci/intx.o\
	driver/virtio/virtio-blk.o\
	hal/block.o\
	filesystem/fat32/mount.o\
	filesystem/vfs/path.o\
	filesystem/fat32/dir.o\
	filesystem/fat32/cluster.o\
	filesystem/fat32/fat.o\
	driver/virtio/virtio.o\
	proc/exec/dynamic.o\
	proc/kcall.o\
	driver/bochs-display/bochs-display.o\
	hal/display.o\
	driver/ps2/mouse.o\
	driver/ps2/keyboard.o\
	proc/pty.o\
	driver/pci/intel-pcie-mmcfg.o\
	driver/pci/pci-legacy.o\
	driver/ata/adapter.o\
	driver/ata/ata.o\
	driver/pci/msi.o\
	driver/pci/driver.o\
	hal/mbr.o\
	hal/module.o\
	hal/hid.o\
	driver/uhci/uhci.o\
	driver/usb/usb.o\
	driver/pci/pci-kcall.o\
	hal/power.o\
	driver/ioapic.o\
	driver/uart.o\
	driver/rtc.o\
	driver/usb/hub.o\
	driver/usb/transfer.o\
	driver/usb/request.o\

CFLAGS += -I. -fno-builtin
ASFLAGS += -I.

kernel : $(OBJS) core/entry.o entryother initcode kernel.ld
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel core/entry.o $(OBJS) $(shell $(CC) -print-libgcc-file-name) -b binary initcode entryother

vectors.S: ../tools/vectors.pl
	../tools/vectors.pl > vectors.S

entryother: core/entryother.S
	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c core/entryother.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother

initcode: proc/init/initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c proc/init/initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode

.PHONY: clean
clean:
	rm -f $(OBJS)  vectors.S kernel entryother bootblockother.o initcode.out initcode core/entry.o entryother.o initcode.o
