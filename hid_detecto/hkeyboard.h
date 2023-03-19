#pragma once
#include "ntdefines.h"
typedef void(__fastcall* fnKeyboardClassServiceCallback)(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed);
NTSTATUS getKeyboardClassServiceCallback(PVOID moduleBase, PVOID* pOutAddr);
NTSTATUS installKeyboardHook();
NTSTATUS uninstallKeyboardHook();