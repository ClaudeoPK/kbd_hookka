#include "utils.h"
#include <ntimage.h>
#include <windef.h>
PCHAR LowerStr(PCHAR str) {
	for (PCHAR s = str; *s; ++s) {
		*s = (CHAR)tolower(*s);
	}
	return str;
}
BOOL CheckMask(PCHAR base, PCHAR pattern, PCHAR mask) {
	for (; *mask; ++base, ++pattern, ++mask) {
		if ('x' == *mask && *base != *pattern) {
			return FALSE;
		}
	}

	return TRUE;
}

PVOID FindPattern(PCHAR base, ULONG length, PCHAR pattern, PCHAR mask) {
	length -= (DWORD)strlen(mask);
	for (DWORD i = 0; i <= length; ++i) {
		PVOID addr = &base[i];
		if (CheckMask(addr, pattern, mask)) {
			return addr;
		}
	}
	return 0;
}
PVOID FindPatternImage(PCHAR base, PCHAR pattern, PCHAR mask) {
	PVOID match = 0;
	PIMAGE_NT_HEADERS headers = (PIMAGE_NT_HEADERS)(base + ((PIMAGE_DOS_HEADER)base)->e_lfanew);
	PIMAGE_SECTION_HEADER sections = IMAGE_FIRST_SECTION(headers);
	for (DWORD i = 0; i < headers->FileHeader.NumberOfSections; ++i) {
		PIMAGE_SECTION_HEADER section = &sections[i];
		if ('EGAP' == *(PINT)section->Name || memcmp(section->Name, ".text", 5) == 0) {
			match = FindPattern(base + section->VirtualAddress, section->Misc.VirtualSize, pattern, mask);
			if (match) {
				break;
			}
		}
	}
	return match;
}
PSYSTEM_MODULE_INFORMATION GetSystemModuleInformation() {
	ULONG szModule = 0;
	NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, 0, 0, &szModule);
	if (STATUS_INFO_LENGTH_MISMATCH != status) {
		DEBUG_OUTPUT("ZwQuerySystemInformation for size failed: %p !\n", status);
		return NULL;
	}
	DEBUG_OUTPUT("ZwQuerySystemInformation size: %d\n", szModule);
	PSYSTEM_MODULE_INFORMATION pBuffer = ExAllocatePool(NonPagedPool, szModule);
	if (!pBuffer) {
		DEBUG_OUTPUT("Failed to allocate %d bytes for modules !\n", szModule);
		return NULL;
	}

	if (!NT_SUCCESS(status = ZwQuerySystemInformation(SystemModuleInformation, pBuffer, szModule, 0))) {
		ExFreePool(pBuffer);
		DEBUG_OUTPUT("ZwQuerySystemInformation failed: %p !\n", status);
		return NULL;
	}
	return pBuffer;
}
PVOID GetBaseAddress(IN PCHAR pModuleName, OUT PULONG pSize) {
	PVOID pModuleBase = NULL;
	PSYSTEM_MODULE_INFORMATION pBuffer = GetSystemModuleInformation();
	if (!pBuffer) {
		DEBUG_OUTPUT("GetSystemModuleInformation failed.\n");
		return pModuleBase;
	}
	for (int i = 0; i < pBuffer->NumberOfModules; i++) {
		if (strstr(LowerStr((PCHAR)pBuffer->Modules[i].FullPathName), pModuleName)) {
			pModuleBase = pBuffer->Modules[i].ImageBase;
			if (pSize) {
				*pSize = pBuffer->Modules[i].ImageSize;
			}
			break;
		}
	}
	ExFreePool(pBuffer);
	return pModuleBase;
}

NTSTATUS GetModuleFullPathNameByRegion(IN PSYSTEM_MODULE_INFORMATION pSystemModuleInformations, IN PVOID Address, OUT CHAR* oBuffer) {
	if (!pSystemModuleInformations || !Address) {
		return STATUS_UNSUCCESSFUL;
	}
	for (int i = 0; i < pSystemModuleInformations->NumberOfModules; i++) {
		if (Address > (ULONG64)pSystemModuleInformations->Modules[i].ImageBase &&
			Address < (ULONG64)pSystemModuleInformations->Modules[i].ImageBase + pSystemModuleInformations->Modules[i].ImageSize) {
			strcpy(oBuffer, pSystemModuleInformations->Modules[i].FullPathName);
			return STATUS_SUCCESS;
		}
	}
	return STATUS_UNSUCCESSFUL;
}
