#include <ntddk.h>
#include <wdf.h>




#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED,FILE_WRITE_DATA | FILE_READ_DATA)  // 07:57
#define DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

DRIVER_INITIALIZE DriverEntry;

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\mydevice");         // 07:57
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\mydevicelink");

PDEVICE_OBJECT DeviceObject = NULL;
 

VOID Unload(PDRIVER_OBJECT DriverObject) {
	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(DeviceObject);
	DbgPrint(("driver unload \r\n"));
}

NTSTATUS DispatchPassThru(PDEVICE_OBJECT Device_Object, PIRP Irp) {   // 1:44

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;

	switch (irpsp->MajorFunction) {
	case IRP_MJ_CREATE:
		DbgPrint(("create request\r\n"));
		break;
	case IRP_MJ_CLOSE:
		DbgPrint(("close request\r\n"));
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}
	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

VOID toggleCase(PUNICODE_STRING str)
{
	for (int i = 0; i < str->Length / 2; i++)
	{

		WCHAR c = str->Buffer[i];
		if ((c >= L'a') && (c <= L'z'))
		{
			str->Buffer[i] = c - L'a' + L'A';
		}
		else
		{
			if ((c >= L'A') && (c <= L'Z'))
			{
				str->Buffer[i] = c - L'A' + L'a';
			}
		}
	}
}

typedef struct _UNICODE_STRING_ENTRY
{
	UNICODE_STRING UnicodeString;
	LIST_ENTRY ListEntry;
}UNICODE_STRING_ENTRY, *PUNICODE_STRING_ENTRY;

LIST_ENTRY UnicodeStringList = { 0 };

VOID AddUnicodeStringToList(PUNICODE_STRING pUnicodeString)
{
	PUNICODE_STRING_ENTRY pEntry = (PUNICODE_STRING_ENTRY)ExAllocatePoolWithTag(NonPagedPool, sizeof(UNICODE_STRING_ENTRY), 'USLT');
	    //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%wZ ", pUnicodeString);
		pEntry->UnicodeString.Buffer = (PWCH)ExAllocatePoolWithTag(NonPagedPool, pUnicodeString->Length,'mytg');
		pEntry->UnicodeString.Length = pUnicodeString->Length;
				
			//RtlCopyMemory(pEntry->UnicodeString.Buffer, pUnicodeString->Buffer, pUnicodeString->Length);

		RtlInitUnicodeString(&pEntry->UnicodeString, pUnicodeString->Buffer);

			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%wZ ", &(pEntry->UnicodeString));
			InsertTailList(&UnicodeStringList, &(pEntry->ListEntry));
	
	
}

VOID PrintUnicodeString()
{
	PLIST_ENTRY pListEntry = UnicodeStringList.Flink;
	while (pListEntry != &UnicodeStringList)
	{
		PUNICODE_STRING_ENTRY pUnicodeStringEntry = CONTAINING_RECORD(pListEntry, UNICODE_STRING_ENTRY, ListEntry);

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%wZ ", &(pUnicodeStringEntry->UnicodeString));
		DbgPrint(" \n");
		pListEntry = pListEntry->Flink;
	}
}


VOID ClearUnicodeStringList()
{
	PLIST_ENTRY pListEntry = UnicodeStringList.Flink;
	while (pListEntry != &UnicodeStringList)
	{
		PUNICODE_STRING_ENTRY pUnicodeStringEntry = CONTAINING_RECORD(pListEntry, UNICODE_STRING_ENTRY, ListEntry);
		RemoveEntryList(pListEntry);
		ExFreePoolWithTag(pUnicodeStringEntry, 'USLT');
		pListEntry = UnicodeStringList.Flink;
	}

}



NTSTATUS DispatchDevCTL(PDEVICE_OBJECT Device_Object, PIRP Irp) {    // 16:58

	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG returnLength = 0;
	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;

	ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLength = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	UNICODE_STRING usString;
	switch (irpsp->Parameters.DeviceIoControl.IoControlCode) {
	case DEVICE_SEND:
		
	
			//khoi tao chuoi Unicode
			RtlInitUnicodeString(&usString, NULL);
			
			//cap phat bo nho cho chuoi
			usString.Buffer = (PWSTR)ExAllocatePoolWithTag(PagedPool, inLength, 'MYTL');
			if (usString.Buffer == NULL)
			{
				return STATUS_INSUFFICIENT_RESOURCES;
			}
			// sao chep du lieu tu buffer sang chuoi Unicode
			RtlCopyMemory(usString.Buffer, buffer, inLength);
			usString.Length = (USHORT)inLength;
			usString.MaximumLength = (USHORT)inLength;
			if (usString.Buffer[0] == '$')
			{
				PrintUnicodeString();
				ClearUnicodeStringList();
				break;
			}
			toggleCase(&usString);

			AddUnicodeStringToList(&usString);
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%wZ \r\n", &usString);
			DbgPrint(" \n");
		
			RtlCopyMemory(buffer, usString.Buffer, inLength);
			//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%ws \r\n", buffer);
			//UnicodeStringtoWChar(usString, sendapp);
			returnLength = (wcsnlen(buffer, 511) + 1) * 2;

		
			
		break;
	case DEVICE_REC:
		
		//RtlCopyUnicodeString(buffer, &usString);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%ws \r\n", bufferout);
		//Irp->AssociatedIrp.SystemBuffer = bufferout;
		/*PLIST_ENTRY pEntry = UnicodeStringList.Flink;
		PUNICODE_STRING_ENTRY pUnicodeStringEntry = CONTAINING_RECORD(pEntry, UNICODE_STRING_ENTRY, ListEntry);
		RtlCopyMemory(buffer, &pUnicodeStringEntry->UnicodeString,&pUnicodeStringEntry->UnicodeString.Length);*/
		//RltCopyMemory(buffer, sendapp->Buffer,sendapp->Length);
		//returnLength = (wcsnlen(bufferout, 511) + 1) * 2;
	
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = returnLength;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath) {  // 1:51

	NTSTATUS status;
	int i;
	

	DriverObject->DriverUnload = Unload;
	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status)) {
		DbgPrint(("creating device failed \r\n"));
		return status;
	}

	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);

	if (!NT_SUCCESS(status)) {
		DbgPrint(("creating symbolic link failed \r\n"));
		IoDeleteDevice(DeviceObject);
		return status;
	}

	InitializeListHead(&UnicodeStringList);


	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPassThru;
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDevCTL;
	DbgPrint(("driver load \r\n"));
	return status;
}