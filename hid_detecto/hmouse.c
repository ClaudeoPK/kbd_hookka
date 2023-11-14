#include "hmouse.h"
#include "utils.h"
fnMouseClassServiceCallback pMouseClassServiceCallback = NULL;
fnMouseClassServiceCallback hkBridgeMouseClassServiceCallback = NULL;
unsigned char OrigOpcodes[12];
void __fastcall hkMouseClassServiceCallback(PDEVICE_OBJECT DeviceObject, PMOUSE_INPUT_DATA InputDataStart, PMOUSE_INPUT_DATA InputDataEnd, PULONG InputDataConsumed) {
	CHAR buffer[256];
	NTSTATUS status = GetModuleFullPathNameByRegion(GetSystemModuleTable(), _ReturnAddress(), buffer);
	for (int i = 0; i < (InputDataEnd - InputDataStart); i++) {
		if (NT_SUCCESS(status)) {
			DEBUG_OUTPUT("Caller:%s\n", buffer);
		}
		else {
			DEBUG_OUTPUT("ReturnAddress:%p\n", _ReturnAddress());
		}
	}
	return hkBridgeMouseClassServiceCallback(DeviceObject, InputDataStart, InputDataEnd, InputDataConsumed);
}
NTSTATUS installMouseHook() {
	if (hkBridgeMouseClassServiceCallback)
		return STATUS_SUCCESS;
	NTSTATUS status = STATUS_SUCCESS;
	ULONG szModule = 0;

	PVOID mouclass = GetBaseAddress("mouclass.sys", &szModule);
	if (!mouclass) {
		DEBUG_OUTPUT("Failed to find mouclass.\n");
		return STATUS_UNSUCCESSFUL;
	}

	status = getMouseClassServiceCallback(mouclass, &pMouseClassServiceCallback);
	if (status != STATUS_SUCCESS) {
		DEBUG_OUTPUT("Failed to find MouseClassServiceCallback.\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (!MmIsAddressValid(pMouseClassServiceCallback)) {
		DEBUG_OUTPUT("Failed to verify MouseClassServiceCallback.\n");
		return STATUS_UNSUCCESSFUL;
	}

	DEBUG_OUTPUT("Found:%p.\n", pMouseClassServiceCallback);

	PHYSICAL_ADDRESS PAMouseClassServiceCallback = MmGetPhysicalAddress(pMouseClassServiceCallback);

	if (PAMouseClassServiceCallback.QuadPart)
	{

		PVOID VAMouseClassServiceCallback = MmMapIoSpaceEx(PAMouseClassServiceCallback, 1024, 4i64);
		if (VAMouseClassServiceCallback)
		{
			BOOLEAN bFound = FALSE;
			if (!hkBridgeMouseClassServiceCallback) {
				int i;
				for (i = 0; i < 64; i++) {
					unsigned char Opcode = *(unsigned char*)((ULONG64)VAMouseClassServiceCallback + i);
					if (Opcode == 0x55) {
						bFound = TRUE;
						break;
					}
				}
				if (bFound) {
					GetSystemModuleInformation();
					unsigned char JumpOrig[] = { 0x48, 0xB8, 0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, /* mov rax, hkFunctionAddress*/
											0xFF, 0xE0 /* jmp rax */ };
					hkBridgeMouseClassServiceCallback = ExAllocatePool(NonPagedPool, 1024);
					RtlCopyMemory(hkBridgeMouseClassServiceCallback, (PVOID)pMouseClassServiceCallback, i);
					*(PVOID*)(JumpOrig + 2) = (ULONG64)pMouseClassServiceCallback + i;
					RtlCopyMemory((ULONG64)hkBridgeMouseClassServiceCallback + i, JumpOrig, sizeof(JumpOrig));
					{
						unsigned char BT[] = { 0x48, 0xB8, 0x00,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0x00, /* mov rax, hkFunctionAddress*/
										0xFF, 0xE0 /* jmp rax */ };
						*(PVOID*)(BT + 2) = hkMouseClassServiceCallback;
						RtlCopyMemory(OrigOpcodes, VAMouseClassServiceCallback, sizeof(BT));
						RtlCopyMemory(VAMouseClassServiceCallback, BT, sizeof(BT));
						MmUnmapIoSpace(VAMouseClassServiceCallback, 1024);
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
NTSTATUS uninstallMouseHook() {
	if (pMouseClassServiceCallback && hkBridgeMouseClassServiceCallback) {
		PHYSICAL_ADDRESS PAMouseClassServiceCallback = MmGetPhysicalAddress(pMouseClassServiceCallback);
		if (PAMouseClassServiceCallback.QuadPart)
		{
			PVOID VAMouseClassServiceCallback = MmMapIoSpaceEx(PAMouseClassServiceCallback, 1024, 4i64);
			if (VAMouseClassServiceCallback) {
				RtlCopyMemory(VAMouseClassServiceCallback, OrigOpcodes, sizeof(OrigOpcodes));
				MmUnmapIoSpace(VAMouseClassServiceCallback, 1024);
				ExFreePool(hkBridgeMouseClassServiceCallback);
				hkBridgeMouseClassServiceCallback = NULL;
				return STATUS_SUCCESS;
			}
		}

	}
	return STATUS_UNSUCCESSFUL;
}
NTSTATUS getMouseClassServiceCallback(PVOID moduleBase, PVOID* pOutAddr) {
	if (!moduleBase) {
		return STATUS_UNSUCCESSFUL;
	}
	PVOID pFunctionRef = FindPatternImage(moduleBase, "\xB9\x03\x02\x0F\x00\x48\x8D\x05", "xxxxxxxx"); // 19045.2251
	if (pFunctionRef) {
		PVOID pMouseClassServiceCallback = (ULONG64)pFunctionRef + 0x5/*pre-instruction length*/ + 0x7/*instruction length*/ + *(INT32*)((ULONG64)pFunctionRef + 0x5 + 0x3);
		*pOutAddr = pMouseClassServiceCallback;
		return STATUS_SUCCESS;
	}
	return STATUS_UNSUCCESSFUL;
}
