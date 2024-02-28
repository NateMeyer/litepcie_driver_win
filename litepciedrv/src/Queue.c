/* SPDX-License-Identifier: BSD-2-Clause
 *
 * LitePCIe Windows Driver
 *
 * Copyright (C) 2023 / Nate Meyer / Nate.Devel@gmail.com
 *
 */
#include <wchar.h>

#include "Driver.h"
#include "litepcie.h"
#include "Queue.tmh"

#include "Trace.h"


#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, litepciedrvQueueInitialize)
#endif

NTSTATUS litepciedrvQueueInitialize(
    _In_ WDFDEVICE Device
)
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
         &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDeviceControl = litepciedrvEvtIoDeviceControl;
    queueConfig.EvtIoRead = litepciedrvEvtIoRead;
    queueConfig.EvtIoWrite = litepciedrvEvtIoWrite;
    queueConfig.EvtIoStop = litepciedrvEvtIoStop;

    status = WdfIoQueueCreate(
                 Device,
                 &queueConfig,
                 WDF_NO_OBJECT_ATTRIBUTES,
                 &queue
                 );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

VOID litepciedrvEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t length = 0;
    PFILE_CONTEXT fileCtx = GetFileContext(WdfRequestGetFileObject(Request));

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %d", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);

    switch (IoControlCode) {
    case LITEPCIE_IOCTL_REG:
        struct litepcie_ioctl_reg *pRegInData, *pRegOutData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_reg), (PVOID*)&pRegInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_reg))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else if (pRegInData->is_write)
            {
                litepciedrv_RegWritel(fileCtx->ctx, pRegInData->addr, pRegInData->val);
                length = 0;
            }
            else
            {
                status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct litepcie_ioctl_reg), (PVOID*)&pRegOutData, &length);
                if (status == STATUS_SUCCESS)
                {
                    pRegOutData->val = litepciedrv_RegReadl(fileCtx->ctx, pRegInData->addr);

                    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE,
                        "litepciedrv REG READ ADDR 0x%X VAL 0x%X", pRegInData->addr, pRegOutData->val);
                }
            }
        }
        break;
#ifdef CSR_FLASH_BASE
    case LITEPCIE_IOCTL_FLASH:
        struct litepcie_ioctl_flash *pFlashInData, *pFlashOutData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_flash), (PVOID*)&pFlashInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_flash))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else if (pFlashInData->tx_len < 8 || pFlashInData->tx_len > 40)
            {
                status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct litepcie_ioctl_flash), (PVOID*)&pFlashOutData, &length);
                if (status == STATUS_SUCCESS)
                {
                    litepciedrv_RegWritel(fileCtx->ctx, CSR_FLASH_SPI_MOSI_ADDR, (UINT32)(pFlashInData->tx_data >> 32));
                    litepciedrv_RegWritel(fileCtx->ctx, CSR_FLASH_SPI_MOSI_ADDR + 4, (UINT32)pFlashInData->tx_data);
                    litepciedrv_RegWritel(fileCtx->ctx, CSR_FLASH_SPI_CONTROL_ADDR,
                        SPI_CTRL_START | (pFlashInData->tx_len * SPI_CTRL_LENGTH));
                    KeStallExecutionProcessor(16);
                    for (UINT32 i = 0; i < SPI_TIMEOUT; i++) {
                        if (litepciedrv_RegReadl(fileCtx->ctx, CSR_FLASH_SPI_STATUS_ADDR) & SPI_STATUS_DONE)
                            break;
                        KeStallExecutionProcessor(1);
                    }
                    pFlashOutData->rx_data = ((UINT64)litepciedrv_RegReadl(fileCtx->ctx, CSR_FLASH_SPI_MISO_ADDR) << 32) |
                        litepciedrv_RegReadl(fileCtx->ctx, CSR_FLASH_SPI_MISO_ADDR + 4);
                    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE,
                        "litepciedrv FLASH TX 0x%llX RX 0x%llX", pFlashInData->tx_data, pFlashOutData->rx_data);
                }
            }
        }
        break;
#endif
    case LITEPCIE_IOCTL_ICAP:
        struct litepcie_ioctl_icap* pIcapInData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_icap), (PVOID*)&pIcapInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_icap))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE,
                        "litepciedrv ICAP ADDR 0x%X DATA 0x%X", pIcapInData->addr, pIcapInData->data);
                    litepciedrv_RegWritel(fileCtx->ctx, CSR_ICAP_ADDR_ADDR, pIcapInData->addr);
                    litepciedrv_RegWritel(fileCtx->ctx, CSR_ICAP_DATA_ADDR, pIcapInData->data);
                    litepciedrv_RegWritel(fileCtx->ctx, CSR_ICAP_WRITE_ADDR, 1);
                    length = 0;
            }
        }
        break;
    case LITEPCIE_IOCTL_DMA:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        struct litepcie_ioctl_dma* pDmaInData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_dma), (PVOID*)&pDmaInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_dma))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                litepciedrv_RegWritel(fileCtx->ctx, (fileCtx->dmaChan->dma.base + PCIE_DMA_LOOPBACK_ENABLE_OFFSET), (UINT32)pDmaInData->loopback_enable);
                length = 0;
            }
        }
        break;
    case LITEPCIE_IOCTL_DMA_WRITER:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        struct litepcie_ioctl_dma_writer* pDmaWriterInData, * pDmaWriterOutData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_dma_writer), (PVOID*)&pDmaWriterInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_dma_writer))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct litepcie_ioctl_dma_writer), (PVOID*)&pDmaWriterOutData, &length);
                if (status == STATUS_SUCCESS)
                {
                    if (pDmaWriterInData->enable != fileCtx->dmaChan->dma.writer_enable) {
                        /* enable / disable DMA */
                        if (pDmaWriterInData->enable) {
                            litepcie_dma_writer_start(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->index);
                            litepcie_enable_interrupt(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->dma.writer_interrupt);
                        }
                        else {
                            litepcie_disable_interrupt(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->dma.writer_interrupt);
                            litepcie_dma_writer_stop(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->index);
                        }
                    }

                    fileCtx->dmaChan->dma.writer_enable = pDmaWriterInData->enable;

                    pDmaWriterOutData->hw_count = fileCtx->dmaChan->dma.writer_hw_count;
                    pDmaWriterOutData->sw_count = fileCtx->dmaChan->dma.writer_sw_count;
                }
            }
        }


        break;
    case LITEPCIE_IOCTL_DMA_READER:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        struct litepcie_ioctl_dma_reader * pDmaReaderInData, * pDmaReaderOutData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_dma_reader), (PVOID*)&pDmaReaderInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_dma_reader))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct litepcie_ioctl_dma_reader), (PVOID*)&pDmaReaderOutData, &length);
                if (status == STATUS_SUCCESS)
                {
                    if (pDmaReaderInData->enable != fileCtx->dmaChan->dma.reader_enable) {
                        /* enable / disable DMA */
                        if (pDmaReaderInData->enable) {
                            litepcie_dma_reader_start(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->index);
                            litepcie_enable_interrupt(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->dma.reader_interrupt);
                        }
                        else {
                            litepcie_disable_interrupt(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->dma.reader_interrupt);
                            litepcie_dma_reader_stop(fileCtx->dmaChan->litepcie_dev, fileCtx->dmaChan->index);
                        }
                    }

                    fileCtx->dmaChan->dma.reader_enable = pDmaReaderInData->enable;

                    pDmaReaderOutData->hw_count = fileCtx->dmaChan->dma.reader_hw_count;
                    pDmaReaderOutData->sw_count = fileCtx->dmaChan->dma.reader_sw_count;
                }
            }
        }
        break;
    case LITEPCIE_IOCTL_MMAP_DMA_INFO:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        struct litepcie_ioctl_mmap_dma_info* pDmaInfoOutData;
        if (status == STATUS_SUCCESS)
        {
            status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct litepcie_ioctl_mmap_dma_info), (PVOID*)&pDmaInfoOutData, &length);
            if (status == STATUS_SUCCESS)
            {
                pDmaInfoOutData->dma_tx_buf_offset = 0;
                pDmaInfoOutData->dma_tx_buf_size = DMA_BUFFER_SIZE;
                pDmaInfoOutData->dma_tx_buf_count = DMA_BUFFER_COUNT;

                pDmaInfoOutData->dma_rx_buf_offset = 0;
                pDmaInfoOutData->dma_rx_buf_size = DMA_BUFFER_SIZE;
                pDmaInfoOutData->dma_rx_buf_count = DMA_BUFFER_COUNT;
            }
        }
        break;
    case LITEPCIE_IOCTL_LOCK:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        struct litepcie_ioctl_lock* pDmaLockInData, * pDmaLockOutData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_lock), (PVOID*)&pDmaLockInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_lock))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                status = WdfRequestRetrieveOutputBuffer(Request, sizeof(struct litepcie_ioctl_lock), (PVOID*)&pDmaLockOutData, &length);
                if (status == STATUS_SUCCESS)
                {

                    pDmaLockOutData->dma_reader_status = 1;
                    if (pDmaLockInData->dma_reader_request) {
                        if (fileCtx->dmaChan->dma.reader_lock) {
                            pDmaLockOutData->dma_reader_status = 0;
                        }
                        else {
                            fileCtx->dmaChan->dma.reader_lock = 1;
                            fileCtx->reader = 1;
                        }
                    }
                    if (pDmaLockInData->dma_reader_release) {
                        fileCtx->dmaChan->dma.reader_lock = 0;
                        fileCtx->reader = 0;
                    }

                    pDmaLockOutData->dma_writer_status = 1;
                    if (pDmaLockInData->dma_writer_request) {
                        if (fileCtx->dmaChan->dma.writer_lock) {
                            pDmaLockOutData->dma_writer_status = 0;
                        }
                        else {
                            fileCtx->dmaChan->dma.writer_lock = 1;
                            fileCtx->writer = 1;
                        }
                    }
                    if (pDmaLockInData->dma_writer_release) {
                        fileCtx->dmaChan->dma.writer_lock = 0;
                        fileCtx->writer = 0;
                    }
                }
            }
        }
        break;
    case LITEPCIE_IOCTL_MMAP_DMA_WRITER_UPDATE:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        struct litepcie_ioctl_mmap_dma_update* pDmaWriteUpdateInData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_mmap_dma_update), (PVOID*)&pDmaWriteUpdateInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_mmap_dma_update))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                fileCtx->dmaChan->dma.writer_sw_count = pDmaWriteUpdateInData->sw_count;
                length = 0;
            }
        }

        break;
    case LITEPCIE_IOCTL_MMAP_DMA_READER_UPDATE:
        if (fileCtx->dev != LITEPCIE_DMA)
        {
            //Wrong file type
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        struct litepcie_ioctl_mmap_dma_update* pDmaReadUpdateInData;
        status = WdfRequestRetrieveInputBuffer(Request, sizeof(struct litepcie_ioctl_mmap_dma_update), (PVOID*)&pDmaReadUpdateInData, &length);
        if (status == STATUS_SUCCESS)
        {
            if (length != sizeof(struct litepcie_ioctl_mmap_dma_update))
            {
                status = STATUS_INVALID_BUFFER_SIZE;
            }
            else
            {
                fileCtx->dmaChan->dma.reader_sw_count = pDmaReadUpdateInData->sw_count;
                length = 0;
            }
        }

        break;
    }

    if (length)
    {
        WdfRequestCompleteWithInformation(Request, status, length);
    }
    else
    {
        WdfRequestComplete(Request, status);
    }
}


VOID litepciedrvEvtIoRead(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST request,
    _In_ size_t length
)
// Callback function on Device node ReadFile
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
                "%!FUNC! Queue 0x%p, Request 0x%p Length %d",
                queue, request, (UINT32)length);
    PFILE_CONTEXT fileCtx = GetFileContext(WdfRequestGetFileObject(request));
    if (fileCtx->dev == LITEPCIE_DMA)
    {
        
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE,
            "litepciedrvEvtIoRead Request %p\n", request);

        litepciedrv_ChannelRead(fileCtx->dmaChan, request, length);

        return;
    }

    WdfRequestComplete(request, STATUS_SUCCESS);
}

VOID litepciedrvEvtIoWrite(
    _In_ WDFQUEUE queue,
    _In_ WDFREQUEST request,
    _In_ size_t length
)
// Callback function on Device node write operations
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE,
        "%!FUNC! Queue 0x%p, Request 0x%p Length %d",
        queue, request, (UINT32)length);
    PFILE_CONTEXT fileCtx = GetFileContext(WdfRequestGetFileObject(request));
    if (fileCtx->dev == LITEPCIE_DMA)
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE,
            "litepciedrvEvtIoWrite Request %p\n", request);

        litepciedrv_ChannelWrite(fileCtx->dmaChan, request, length);

        return;
    }

    WdfRequestComplete(request, STATUS_SUCCESS);
}

VOID litepciedrvEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);

    return;
}

VOID litepciedrvEvtDeviceFileCreate(
    _In_ WDFDEVICE device,
    _In_ WDFREQUEST Request,
    _In_ WDFFILEOBJECT WdfFile
)
{
    PUNICODE_STRING fileName = WdfFileObjectGetFileName(WdfFile);
    PDEVICE_CONTEXT ctx = DeviceGetContext(device);
    PFILE_CONTEXT devNode = GetFileContext(WdfFile);
    NTSTATUS status = STATUS_SUCCESS;

    if (fileName == NULL) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Error: no filename given.");
        status = STATUS_INVALID_PARAMETER;
        goto ErrExit;
    }

    if (0 == wcscmp(fileName->Buffer, L"\\CTRL"))
    {
        devNode->dev = LITEPCIE_CTRL;
        devNode->ctx = ctx;
        devNode->dmaChan = NULL;
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE, "Opening LitePCIe REG device");
    }
    else if (0 == wcsncmp(fileName->Buffer, L"\\DMA", 4))
    {
        UINT32 channelId;
        if (1 != swscanf_s((const wchar_t*)fileName->Buffer, L"\\DMA%d", &channelId))
        {
            // Bad name error
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Bad Name Opening LitePCIe DMA device");
            status = STATUS_INVALID_PARAMETER;
            goto ErrExit;
        }
        devNode->dev = LITEPCIE_DMA;
        devNode->ctx = ctx;
        devNode->dmaChan = &ctx->chan[channelId];

        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE, "Opening LitePCIe DMA device");
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "Error: Unknown Name.");
    }

ErrExit:
    WdfRequestComplete(Request, status);
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE, "returns %!STATUS!", status);
}

VOID litepciedrvEvtFileClose(IN WDFFILEOBJECT FileObject) {
    PUNICODE_STRING fileName = WdfFileObjectGetFileName(FileObject);
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_QUEUE, "Closing file %wZ", fileName);
}

VOID litepciedrvEvtFileCleanup(IN WDFFILEOBJECT FileObject) {
    PUNICODE_STRING fileName = WdfFileObjectGetFileName(FileObject);
    PFILE_CONTEXT file = GetFileContext(FileObject);

    if (file->dev == LITEPCIE_CTRL)
    {

    }
    if (file->dev == LITEPCIE_DMA)
    {
        if (file->reader)
        {
            //Unlock reader
            file->dmaChan->dma.reader_lock = 0;
        }
        if (file->writer)
        {
            //Unlock writer
            file->dmaChan->dma.writer_lock = 0;
        }
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_QUEUE, "Cleanup %wZ", fileName);
}
