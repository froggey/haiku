#ifndef KERNEL_BOOT_PLATFORM_EFI_KERNEL_ARGS_H
#define KERNEL_BOOT_PLATFORM_EFI_KERNEL_ARGS_H

#ifndef KERNEL_BOOT_KERNEL_ARGS_H
#	error This file is included from <boot/kernel_args.h> only
#endif

#warning EFI: IMPLEMENT platform_kernel_args.h

#define SMP_MAX_CPUS 64

#define MAX_PHYSICAL_MEMORY_RANGE 32
#define MAX_PHYSICAL_ALLOCATED_RANGE 32
#define MAX_VIRTUAL_ALLOCATED_RANGE 32

#define MAX_SERIAL_PORTS 4

typedef struct {
	uint16		serial_base_ports[MAX_SERIAL_PORTS];

//	FixedWidthPointer<bios_drive> drives;
		// this does not contain the boot drive

//	apm_info	apm;
} _PACKED platform_kernel_args;

#endif	/* KERNEL_BOOT_PLATFORM_BIOS_IA32_KERNEL_ARGS_H */
