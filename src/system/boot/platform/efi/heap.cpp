/*
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */

#include "efi_platform.h"

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#define STAGE_PAGES     0x2000   /* 32 MB */
#define PAGE_SIZE			0x1000 /* 4kB */


EFI_PHYSICAL_ADDRESS    staging;


void
platform_release_heap(struct stage2_args *args, void *base)
{
	ASSERT((void*)staging == base);
    kSystemTable->BootServices->FreePages(staging, STAGE_PAGES);
}


status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
	if (kSystemTable->BootServices->AllocatePages(
		AllocateAnyPages, EfiLoaderData, STAGE_PAGES, &staging)
			!= EFI_SUCCESS)
		return B_NO_MEMORY;
	
    kprintf("heap address = %lx, size = %d pages\n", staging, STAGE_PAGES);
    
    *_base = (void *)staging;
    *_top = (void *)((int8 *)staging + STAGE_PAGES * PAGE_SIZE);
	kprintf("heap size = %ld\n", (uint64)*_top - (uint64)*_base);
    kprintf("heap: base = %lx, top = %lx\n", (uint64)*_base, (uint64)*_top);
    return B_OK;
}


extern "C" status_t
platform_allocate_region(void **_address, size_t size, uint8 protection, bool /* exactAddress */)
{
	EFI_STATUS status;
	
	status = kSystemTable->BootServices->
		AllocatePool(EfiLoaderData, size, _address);
	if (status != EFI_SUCCESS) {
		kprintf("platform_allocate_region: %d\n", status);
		return B_NO_MEMORY;
	}
	
	return B_OK;
}


extern "C" status_t
platform_free_region(void *address, size_t /* size */)
{
	kSystemTable->BootServices->FreePool(address);
		
	return B_OK;
}
