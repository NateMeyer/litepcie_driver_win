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
#include <initguid.h>

#include "Device.h"
#include "Queue.h"
#include "Trace.h"

EXTERN_C_START

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD litepciedrvEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP litepciedrvEvtDriverContextCleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE litepciedrvEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE litepciedrvEvtDeviceReleaseHardware;

EXTERN_C_END
