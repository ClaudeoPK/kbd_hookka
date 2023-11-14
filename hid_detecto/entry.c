#include "ntdefines.h"
#include "hkeyboard.h"
#include "hmouse.h"
#include "utils.h"

UNICODE_STRING DEVICE_NAME;
UNICODE_STRING DOS_DEVICE_NAME;
PDEVICE_OBJECT pDeviceObject = NULL;
NTSTATUS UnloadDriver(PDRIVER_OBJECT pDriverObject)
{
	uninstallKeyboardHook();
	FreeSystemModuleTable();
	DEBUG_OUTPUT("Unloading Driver...\n");
	IoDeleteDevice(pDriverObject->DeviceObject);
}
NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING regPath) {
	RtlInitUnicodeString(&DEVICE_NAME,		L"\\Device\\hiddetecto");
	RtlInitUnicodeString(&DOS_DEVICE_NAME,  L"\\DosDevices\\hiddetecto");

	IoCreateDevice(pDriverObject, 0, &DEVICE_NAME, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &pDeviceObject);
	pDriverObject->DriverUnload = UnloadDriver;
	pDeviceObject->Flags |= DO_DIRECT_IO;
	pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

	InitializeSystemModuleTable();
	installKeyboardHook();
	installMouseHook();

}