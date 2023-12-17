/* SPDX-License-Identifier: BSD-2-Clause
 *
 * LitePCIe Windows Driver
 *
 * Copyright (C) 2023 / Nate Meyer / Nate.Devel@gmail.com
 *
 */

#pragma once

EXTERN_C_START

typedef enum {
    LITEPCIE_CTRL,
    LITEPCIE_DMA
} LITEPCIEDEV;

typedef struct FILE_CONTEXT_S {
    LITEPCIEDEV dev;
    WDFQUEUE queue;
    PDEVICE_CONTEXT ctx;
    PLITEPCIE_CHAN dmaChan;
    UINT8 reader;
    UINT8 writer;
}FILE_CONTEXT, *PFILE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILE_CONTEXT, GetFileContext)
//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

NTSTATUS
litepciedrvQueueInitialize(
    _In_ WDFDEVICE Device
    );


// File IO Event Handlers
EVT_WDF_DEVICE_FILE_CREATE          litepciedrvEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE                  litepciedrvEvtFileClose;
EVT_WDF_FILE_CLEANUP                litepciedrvEvtFileCleanup;

//
// Events from the IoQueue object
//
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL litepciedrvEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_STOP litepciedrvEvtIoStop;
EVT_WDF_IO_QUEUE_IO_READ litepciedrvEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE litepciedrvEvtIoWrite;

EXTERN_C_END
