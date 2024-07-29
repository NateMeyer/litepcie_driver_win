/* SPDX-License-Identifier: BSD-2-Clause
 *
 * LitePCIe Windows Driver
 *
 * Copyright (C) 2023 / Nate Meyer / Nate.Devel@gmail.com
 *
 */
#pragma once

#include <ntddk.h>
#include <wdf.h>
#include "litepcie.h"

#include "litepcie_dmadrv.h"
#include "csr.h"


EXTERN_C_START

struct litepcie_dma_chan {
    UINT32 base;
    UINT32 reader_interrupt;
    UINT32 writer_interrupt;
    WDFSPINLOCK readerLock;
    WDFSPINLOCK writerLock;
    WDFREQUEST readRequest;
    SIZE_T readBytes;
    SIZE_T readReqBytes;
    WDFREQUEST writeRequest;
    SIZE_T writeBytes;
    SIZE_T writeReqBytes;
    WDFCOMMONBUFFER readBuffer;
    WDFCOMMONBUFFER writeBuffer;
    PVOID reader_handle[DMA_BUFFER_COUNT];
    PVOID writer_handle[DMA_BUFFER_COUNT];
    PHYSICAL_ADDRESS reader_addr[DMA_BUFFER_COUNT];
    PHYSICAL_ADDRESS writer_addr[DMA_BUFFER_COUNT];
    volatile INT64 reader_hw_count;
    volatile INT64 reader_hw_count_last;
    INT64 reader_sw_count;
    volatile INT64 writer_hw_count;
    volatile INT64 writer_hw_count_last;
    INT64 writer_sw_count;
    UINT8 writer_enable;
    UINT8 reader_enable;
    UINT8 reader_lock;
    UINT8 writer_lock; 
};

typedef struct litepcie_chan {
    struct _DEVICE_CONTEXT* litepcie_dev;
    struct litepcie_dma_chan dma;
    UINT32 block_size;
    UINT32 index;
}LITEPCIE_CHAN, *PLITEPCIE_CHAN;

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    WDFDEVICE deviceDrv;
    ULONG bar0_size;
    PVOID bar0_phys_addr;
    PVOID bar0_addr; /* virtual address of BAR0 */
    struct litepcie_chan chan[DMA_CHANNEL_COUNT];
    WDFSPINLOCK dmaLock;
    WDFDMAENABLER dmaEnabler;
    WDFDMATRANSACTION dmaTransaction;
    UINT32 irqs;
    WDFINTERRUPT intr;
    UINT32 irqs_requested;
    UINT32 irqs_pending;
    UINT32 channels;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called DeviceGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS litepciedrvCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit);

EVT_WDF_OBJECT_CONTEXT_CLEANUP litepciedrvCleanupDevice;

NTSTATUS litepciedrv_DeviceOpen(WDFDEVICE wdfDevice,
    PDEVICE_CONTEXT litepcie,
    WDFCMRESLIST ResourcesRaw,
    WDFCMRESLIST ResourcesTranslated);

NTSTATUS litepciedrv_DeviceClose(WDFDEVICE wdfDevice);

UINT32 litepciedrv_RegReadl(PDEVICE_CONTEXT dev, UINT32 reg);

VOID litepciedrv_RegWritel(PDEVICE_CONTEXT dev, UINT32 reg, UINT32 val);

VOID litepciedrv_ChannelRead(PLITEPCIE_CHAN channel, WDFREQUEST request, SIZE_T length);

VOID litepciedrv_ChannelReadCancel(WDFREQUEST request);

VOID litepciedrv_ChannelWrite(PLITEPCIE_CHAN channel, WDFREQUEST request, SIZE_T length);

VOID litepciedrv_ChannelWriteCancel(WDFREQUEST request);

VOID litepcie_dma_writer_start(PDEVICE_CONTEXT dev, UINT32 index);

VOID litepcie_dma_writer_stop(PDEVICE_CONTEXT dev, UINT32 index);

VOID litepcie_dma_reader_start(PDEVICE_CONTEXT dev, UINT32 index);

VOID litepcie_dma_reader_stop(PDEVICE_CONTEXT dev, UINT32 index);

VOID litepcie_enable_interrupt(PDEVICE_CONTEXT dev, UINT32 interrupt);

VOID litepcie_disable_interrupt(PDEVICE_CONTEXT dev, UINT32 interrupt);

EXTERN_C_END
