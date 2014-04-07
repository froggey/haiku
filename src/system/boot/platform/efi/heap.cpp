/*
 * Copyright 2014, Jessica Hamilton, jessica.l.hamilton@gmail.com.
 *
 * Distributed under the terms of the MIT License.
 */

#include "efi_platform.h"

#include <boot/platform.h>
#include <boot/stage2.h>
#include <boot/stdio.h>

#define STAGE_PAGES     10000   /* 64 MB */

EFI_PHYSICAL_ADDRESS    staging;

void
platform_release_heap(struct stage2_args *args, void *base)
{
    // TODO
}


status_t
platform_init_heap(struct stage2_args *args, void **_base, void **_top)
{
    kSystemTable->BootServices->AllocatePages(
        AllocateAnyPages, EfiLoaderData, STAGE_PAGES, &staging);
    // get_next_physical_address(args->heap_size);
    kprintf("heap address = %lx\n", staging);
    if (staging == 0l)
        return B_NO_MEMORY;
    
    *_base = (void *)staging;
    *_top = (void *)((int8 *)staging + STAGE_PAGES);
    kprintf("heap: base = %lx, top = %lx\n", (uint64)*_base, (uint64)*_top);
    return B_OK;
}
