#include "DeviceIO.h"
#include "../Rule.h"
#include "../Log/Log.h"
#include "../Notification/NotificationQueue.h"
#include "../RegistryFilter.h"

typedef struct _IO_DEVICE_EXTENSION
{
	PVOID ptr;
} IO_DEVICE_EXTENSION, *PIO_DEVICE_EXTENSION;

static UNICODE_STRING uDeviceName;
static UNICODE_STRING uDosDeviceName;
static PDEVICE_OBJECT g_DeviceObject;

_Use_decl_annotations_
NTSTATUS DeviceIOInitialize(
	PDRIVER_OBJECT DriverObject
)
{
	NTSTATUS Status;

	g_DeviceObject = NULL;
	RtlInitUnicodeString(&uDeviceName, DeviceName);
	RtlInitUnicodeString(&uDosDeviceName, DosDeviceName);

	Status = IoCreateDevice(
		DriverObject,
		sizeof(IO_DEVICE_EXTENSION),
		&uDeviceName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_UNKNOWN,
		FALSE,
		&g_DeviceObject
	);

	if (!NT_SUCCESS(Status))
	{
		DEBUG_LOG("CreateIODevice: IoCreateDevice failed with error 0x%.8X", Status);
		return Status;
	}

	g_DeviceObject->Flags |= DO_BUFFERED_IO;

	Status = IoCreateSymbolicLink(
		&uDosDeviceName,
		&uDeviceName
	);

	if (!NT_SUCCESS(Status))
	{
		IoDeleteDevice(g_DeviceObject);
		DEBUG_LOG("CreateIODevice: IoCreateSymbolicLink failed with error 0x%.8X", Status);
		return Status;
	}
	DriverObject->MajorFunction[IRP_MJ_CREATE]			= DeviceIOCreate;
	DriverObject->MajorFunction[IRP_MJ_READ]			= DeviceIORead;
	DriverObject->MajorFunction[IRP_MJ_CLEANUP]			= DeviceIOCleanup;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]			= DeviceIOClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]	= DeviceIOControl;


	DEBUG_LOG("CreateIODevice completed");
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceIOUnload(
	PDRIVER_OBJECT DriverObject
)
{
	IoDeleteSymbolicLink(&uDosDeviceName);
	IoDeleteDevice(g_DeviceObject);
	DEBUG_LOG("DeviceIOUnload completed");
	UNREFERENCED_PARAMETER(DriverObject);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceIOCreate(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceIORead(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
)
{
	PIO_STACK_LOCATION IOStackLocation;
	NTSTATUS Status;
	ULONG BytesRead;

	IOStackLocation = IoGetCurrentIrpStackLocation(Irp);

	BytesRead = 0;

	Status = NotificationCopy(
		Irp->AssociatedIrp.SystemBuffer,
		IOStackLocation->Parameters.Read.Length,
		&BytesRead
	);

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = BytesRead;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	UNREFERENCED_PARAMETER(DeviceObject);
	return Status;
}
NTSTATUS DeviceIOControlAddRule(
	PIRP Irp
)
{
	OBSERVER_RULE_HANDLE RuleHandle = { 0, 0 };
	PIO_STACK_LOCATION IOStackLocation;
	ULONG_PTR Information = 0;
	NTSTATUS Status;
	POBSERVER_ADD_RULE pAddRule;
	ULONG Length;
	ULONG Offset = FIELD_OFFSET(OBSERVER_ADD_RULE, Rule);

	IOStackLocation = IoGetCurrentIrpStackLocation(Irp);
	Length = IOStackLocation->Parameters.DeviceIoControl.InputBufferLength;

	if (Length < Offset)
	{
		Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);
		return STATUS_BUFFER_TOO_SMALL;
	}
	Length -= Offset;
	pAddRule = (POBSERVER_ADD_RULE)Irp->AssociatedIrp.SystemBuffer;

	switch (pAddRule->RuleType)
	{
	case RULE_TYPE_REGISTRY:
	{
		if (Length < sizeof(OBSERVER_REGISTRY_RULE))
		{
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		Length -= FIELD_OFFSET(OBSERVER_REGISTRY_RULE, Path);
		if (Length < pAddRule->Rule.Registry.PathLength)
		{
			Status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		Status = RegistryFilterAddRule(&pAddRule->Rule.Registry, &RuleHandle);
		break;
	}
	default:
		Status = STATUS_NOT_IMPLEMENTED;
		break;
	}

	if (NT_SUCCESS(Status))
	{
		if (IOStackLocation->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(OBSERVER_RULE_HANDLE))
		{
			((POBSERVER_RULE_HANDLE)Irp->AssociatedIrp.SystemBuffer)->RuleHandle = RuleHandle.RuleHandle;
			((POBSERVER_RULE_HANDLE)Irp->AssociatedIrp.SystemBuffer)->RuleType = RuleHandle.RuleType;
			Information = sizeof(OBSERVER_RULE_HANDLE);
		}
	}

	Irp->IoStatus.Status = Status;
	Irp->IoStatus.Information = Information;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return Status;
}

_Use_decl_annotations_
NTSTATUS DeviceIOControl(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
)
{
	PIO_STACK_LOCATION IOStackLocation;
	IOStackLocation = IoGetCurrentIrpStackLocation(Irp);
	if (IOStackLocation->Parameters.DeviceIoControl.IoControlCode ==
		IOCTL_OBSERVER_ADD_RULE)
	{
		return DeviceIOControlAddRule(Irp);
	}

	Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	UNREFERENCED_PARAMETER(DeviceObject);
	return STATUS_NOT_IMPLEMENTED;
}

_Use_decl_annotations_
NTSTATUS DeviceIOCleanup(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS DeviceIOClose(
	PDEVICE_OBJECT	DeviceObject,
	PIRP			Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	UNREFERENCED_PARAMETER(Irp);
	return STATUS_SUCCESS;
}
