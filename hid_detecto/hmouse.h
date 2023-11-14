#pragma once
#include "ntdefines.h"
typedef void(__fastcall* fnMouseClassServiceCallback)(PDEVICE_OBJECT DeviceObject, PMOUSE_INPUT_DATA InputDataStart, PMOUSE_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);
NTSTATUS getMouseClassServiceCallback(PVOID moduleBase, PVOID* pOutAddr);
NTSTATUS installMouseHook();
NTSTATUS uninstallMouseHook();