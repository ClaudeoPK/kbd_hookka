#include "hkeyboard.h"
#include "utils.h"
fnKeyboardClassServiceCallback pKeyboardClassServiceCallback = NULL;
fnKeyboardClassServiceCallback hkBridgeKeyboardClassServiceCallback = NULL;
PSYSTEM_MODULE_INFORMATION pSystemModuleInformations = NULL;
unsigned char OrigOpcodes[12];
void __fastcall hkKeyboardClassServiceCallback(PDEVICE_OBJECT DeviceObject, PKEYBOARD_INPUT_DATA InputDataStart, PKEYBOARD_INPUT_DATA InputDataEnd, PULONG InputDataConsumed) {
	CHAR buffer[256];
	NTSTATUS status = GetModuleFullPathNameByRegion(pSystemModuleInformations, _ReturnAddress(), buffer);
	for (int i = 0; i < (InputDataEnd - InputDataStart); i++) {
		if (NT_SUCCESS(status)) {
			DEBUG_OUTPUT("Caller:%s Scancode : %d,key %s\n", buffer, (InputDataStart + i * sizeof(KEYBOARD_INPUT_DATA))->MakeCode, (InputDataStart + i * sizeof(KEYBOARD_INPUT_DATA))->Flags ? "Up" : "Down");
		}
		else {
			DEBUG_OUTPUT("ReturnAddress:%p Scancode : %d,key %s\n", _ReturnAddress(), (InputDataStart + i * sizeof(KEYBOARD_INPUT_DATA))->MakeCode, (InputDataStart + i * sizeof(KEYBOARD_INPUT_DATA))->Flags ? "Up" : "Down");
		}
		
	}
	return hkBridgeKeyboardClassServiceCallback(DeviceObject, InputDataStart, InputDataEnd, InputDataConsumed);
}
NTSTATUS installKeyboardHook() {
	if (hkBridgeKeyboardClassServiceCallback)
		return STATUS_SUCCESS;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG szModule = 0;

	PVOID kbdclass = GetBaseAddress("kbdclass.sys", &szModule);
	if (!kbdclass) {
		DEBUG_OUTPUT("Failed to find kbdclass.\n");
		return STATUS_UNSUCCESSFUL;
	}

	status = getKeyboardClassServiceCallback(kbdclass, &pKeyboardClassServiceCallback);
	if (status != STATUS_SUCCESS) {
		DEBUG_OUTPUT("Failed to find KeyboardClassServiceCallback.\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (!MmIsAddressValid(pKeyboardClassServiceCallback)) {
		DEBUG_OUTPUT("Failed to verify KeyboardClassServiceCallback.\n");
		return STATUS_UNSUCCESSFUL;
	}

	DEBUG_OUTPUT("Found:%p.\n", pKeyboardClassServiceCallback);
	
	PHYSICAL_ADDRESS PAKeyboardClassServiceCallback = MmGetPhysicalAddress(pKeyboardClassServiceCallback);
	
	if (PAKeyboardClassServiceCallback.QuadPart)
	{

		PVOID VAKeyboardClassServiceCallback = MmMapIoSpaceEx(PAKeyboardClassServiceCallback, 1024, 4i64);
		if (VAKeyboardClassServiceCallback)
		{
			BOOLEAN bFound = FALSE;
			if (!hkBridgeKeyboardClassServiceCallback) {
				int i;
				for (i = 0; i < 64; i++) {
					unsigned char Opcode = *(unsigned char*)((ULONG64)VAKeyboardClassServiceCallback + i);
					if (Opcode == 0x55) {
						bFound = TRUE;
						break;
					}
				}
				if (bFound) {
					pSystemModuleInformations = GetSystemModuleInformation();
					unsigned char JumpOrig[] = { 0x48, 0xB8, 0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, /* mov rax, hkFunctionAddress*/
											0xFF, 0xE0 /* jmp rax */ };
					hkBridgeKeyboardClassServiceCallback = ExAllocatePool(NonPagedPool, 1024);
					RtlCopyMemory(hkBridgeKeyboardClassServiceCallback, (PVOID)pKeyboardClassServiceCallback, i);
					*(PVOID*)(JumpOrig + 2) = (ULONG64)pKeyboardClassServiceCallback + i;
					RtlCopyMemory((ULONG64)hkBridgeKeyboardClassServiceCallback + i, JumpOrig, sizeof(JumpOrig));
					{
						unsigned char BT[] = { 0x48, 0xB8, 0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, /* mov rax, hkFunctionAddress*/
										0xFF, 0xE0 /* jmp rax */ };
						*(PVOID*)(BT + 2) = hkKeyboardClassServiceCallback;
						RtlCopyMemory(OrigOpcodes, VAKeyboardClassServiceCallback, sizeof(BT));
						RtlCopyMemory(VAKeyboardClassServiceCallback, BT, sizeof(BT));
						MmUnmapIoSpace(VAKeyboardClassServiceCallback, 1024);
						status = STATUS_SUCCESS;
						return status;
					}
				}
				else {
					DEBUG_OUTPUT("Failed to install hook[0]\n");
				}

			}
		}
	}
	return STATUS_UNSUCCESSFUL;
}
NTSTATUS uninstallKeyboardHook() {
	if (pSystemModuleInformations) {
		ExFreePool(pSystemModuleInformations);
		pSystemModuleInformations = NULL;
	}
	if (pKeyboardClassServiceCallback && hkBridgeKeyboardClassServiceCallback) {
		PHYSICAL_ADDRESS PAKeyboardClassServiceCallback = MmGetPhysicalAddress(pKeyboardClassServiceCallback);
		if (PAKeyboardClassServiceCallback.QuadPart)
		{
			PVOID VAKeyboardClassServiceCallback = MmMapIoSpaceEx(PAKeyboardClassServiceCallback, 1024, 4i64);
			if (VAKeyboardClassServiceCallback) {
				RtlCopyMemory(VAKeyboardClassServiceCallback, OrigOpcodes, sizeof(OrigOpcodes));
				MmUnmapIoSpace(VAKeyboardClassServiceCallback, 1024);
				ExFreePool(hkBridgeKeyboardClassServiceCallback);
				hkBridgeKeyboardClassServiceCallback = NULL;
				return STATUS_SUCCESS;
			}
		}
		
	}
	return STATUS_UNSUCCESSFUL;
}
NTSTATUS getKeyboardClassServiceCallback(PVOID moduleBase, PVOID* pOutAddr) {
	if (!moduleBase) {
		return STATUS_UNSUCCESSFUL;
	}
	PVOID pFunctionRef = FindPatternImage(moduleBase, "\xB9\x03\x02\x0B\x00\x48\x8D\x05", "xxxxxxxx"); // 19045.2251
	if (pFunctionRef) {
		PVOID pKeyboardClassServiceCallback = (ULONG64)pFunctionRef + 0x5/*pre-instruction length*/ + 0x7/*instruction length*/ + *(INT32*)((ULONG64)pFunctionRef + 0x5 + 0x3);
		*pOutAddr = pKeyboardClassServiceCallback;
		return STATUS_SUCCESS;
	}
	return STATUS_UNSUCCESSFUL;
}
