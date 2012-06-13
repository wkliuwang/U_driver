#include "stddcls.h"
#include "Driver.h"
#include <srb.h>
#include <scsi.h>

//����豸����
NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo);

//�豸ж�غ���
VOID DriverUnload(IN PDRIVER_OBJECT fido);

//��������ǲ����
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp);

//IRP_MJ_POWER��ǲ����
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp);

//IRP_MJ_PNP��ǲ����
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fido, IN PIRP Irp);

//�õ��豸����
ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo);

//���ϴ���û�н��״̬�ĺ�����������Ӧ�ļ���־
NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);

//���ϴ���û�н��״̬�ĺ�����������Ӧflag��־
NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);



///////////////////////////////////////////////////////////////////////////////
#pragma INITCODE
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	DbgPrint(DRIVERNAME " - Entering DriverEntry: DriverObject %8.8lX\n", DriverObject);

	//��ʼ����ǲ����
	DriverObject->DriverUnload = DriverUnload;
	DriverObject->DriverExtension->AddDevice = AddDevice;

	for (int i = 0; i < arraysize(DriverObject->MajorFunction); ++i)
		DriverObject->MajorFunction[i] = DispatchAny;

	DriverObject->MajorFunction[IRP_MJ_POWER] = DispatchPower;
	DriverObject->MajorFunction[IRP_MJ_PNP] = DispatchPnp;
	DriverObject->MajorFunction[IRP_MJ_SCSI] = DispatchForSCSI;

	return STATUS_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////
#pragma PAGEDCODE
VOID DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
    //ʹ�÷�ҳ�ڴ�
	PAGED_CODE();
	DbgPrint(DRIVERNAME " - Entering DriverUnload: DriverObject %8.8lX\n", DriverObject);
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo)
{
    //ȷ����ǰ���������ڷ�ҳ�ڴ�
	PAGED_CODE();
	NTSTATUS status;

	PDEVICE_OBJECT fido;

	//�����豸����
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL,
		GetDeviceTypeToUse(pdo), 0, FALSE, &fido);

    //�ж��Ƿ�ɹ������豸����
	if (!NT_SUCCESS(status))
	{
		DbgPrint(DRIVERNAME " - IoCreateDevice failed - %X\n", status);
		//������ܳɹ������豸�����򷵻�
		return status;
	}

	//��ȡ�豸��չ
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	do
	{
	    //��ʼ��������
		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);
		pdx->DeviceObject = fido;
		pdx->Pdo = pdo;

		//���������������ڵײ�����֮��
		PDEVICE_OBJECT fdo = IoAttachDeviceToDeviceStack(fido, pdo);

		//������Ӳ���ʧ��
		if (!fdo)
		{
			DbgPrint(DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n");
			status = STATUS_DEVICE_REMOVED;
			break;
		}

		//��¼�ײ�����
		pdx->LowerDeviceObject = fdo;

		//���ڲ�֪���ײ�������ֱ��IO����BufferIO����˽���־������
		fido->Flags |= fdo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);

		//�����ʼ����־�������ܹ���ȡIRP
		fido->Flags &= ~DO_DEVICE_INITIALIZING;
	}	while (FALSE);

	//���û�гɹ�
	if (!NT_SUCCESS(status))
	{
	    //����豸ջ��ɾ���豸
		if (pdx->LowerDeviceObject)
			IoDetachDevice(pdx->LowerDeviceObject);
		IoDeleteDevice(fido);
	}

	return status;
}


///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
{
    //������������

    //��ֵ��Ӧ����������ɺ�������
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS
USBSCSICompletion( IN PDEVICE_OBJECT DeviceObject,
                   IN PIRP Irp,
                   IN PVOID Context )
{
    //�ڴ˺���ʵ�ֶ�U���豸��д����

    //��ȡ�豸��չ
    PDEVICE_EXTENSION pdx = ( PDEVICE_EXTENSION )
                                   DeviceObject->DeviceExtension;

    //��ȡ������
	IoAcquireRemoveLock(&pdx->RemoveLock,Irp);

    //��ȡ��ǰIO��ջ
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation( Irp );

    //��ȡ��ǰIRP����״̬
	PSCSI_REQUEST_BLOCK CurSrb=irpStack->Parameters.Scsi.Srb;
	PCDB cdb = (PCDB)CurSrb->Cdb;

	//��ȡ������
	UCHAR opCode=cdb->CDB6GENERIC.OperationCode;

    //�ж��Ƿ���SCSIOP_MODE_SENSE����
	if(opCode==SCSIOP_MODE_SENSE  && CurSrb->DataBuffer
		&& CurSrb->DataTransferLength >=
		sizeof(MODE_PARAMETER_HEADER))
	{
		DbgPrint("SCSIOP_MODE_SENSE comming!\n");

        //�õ�ģʽ����ͷ
		PMODE_PARAMETER_HEADER modeData = (PMODE_PARAMETER_HEADER)CurSrb->DataBuffer;

        //����U��Ϊֻ��״̬������
		modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
	}

    //�ж��Ƿ���Ҫ����
	if ( Irp->PendingReturned )
	{
		IoMarkIrpPending( Irp );
	}

    //�ͷ�������
	IoReleaseRemoveLock(&pdx->RemoveLock,Irp);

	return Irp->IoStatus.Status ;
}

#pragma LOCKEDCODE
NTSTATUS DispatchForSCSI(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //IRP_MJ_SCSI����ǲ����

    //��ȡ�豸��չ
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

    //��ȡ��ǰIO��ջ
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	//���²��豸����IRP
	NTSTATUS status;

	//��ȡ������
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//��ȡʧ��
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

    //����ǰIO��ջ���Ƶ��²�IO��ջ
	IoCopyCurrentIrpStackLocationToNext(Irp);

    //�����������
	IoSetCompletionRoutine( Irp,
							USBSCSICompletion,
							NULL,
							TRUE,
							TRUE,
							TRUE );

    //���õײ��豸������
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	//�ͷ�������
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}


///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //������ǲ����
    //�Բ��������IRPֻ��Ҫ����ֱ�Ӵ���������������

    //��ȡ�豸��չ
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	//��ȡIO��ջ
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

#if DBG
	static char* irpname[] =
	{
		"IRP_MJ_CREATE",
		"IRP_MJ_CREATE_NAMED_PIPE",
		"IRP_MJ_CLOSE",
		"IRP_MJ_READ",
		"IRP_MJ_WRITE",
		"IRP_MJ_QUERY_INFORMATION",
		"IRP_MJ_SET_INFORMATION",
		"IRP_MJ_QUERY_EA",
		"IRP_MJ_SET_EA",
		"IRP_MJ_FLUSH_BUFFERS",
		"IRP_MJ_QUERY_VOLUME_INFORMATION",
		"IRP_MJ_SET_VOLUME_INFORMATION",
		"IRP_MJ_DIRECTORY_CONTROL",
		"IRP_MJ_FILE_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CONTROL",
		"IRP_MJ_INTERNAL_DEVICE_CONTROL",
		"IRP_MJ_SHUTDOWN",
		"IRP_MJ_LOCK_CONTROL",
		"IRP_MJ_CLEANUP",
		"IRP_MJ_CREATE_MAILSLOT",
		"IRP_MJ_QUERY_SECURITY",
		"IRP_MJ_SET_SECURITY",
		"IRP_MJ_POWER",
		"IRP_MJ_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CHANGE",
		"IRP_MJ_QUERY_QUOTA",
		"IRP_MJ_SET_QUOTA",
		"IRP_MJ_PNP",
	};//����IRP��������

    //��ȡIRP��������
	UCHAR type = stack->MajorFunction;
#endif

	NTSTATUS status;

	//��ȡ������
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//�Ƿ�ɹ���ȡ
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

    //�Թ���ǰIO��ջ
	IoSkipCurrentIrpStackLocation(Irp);

	//���õײ���������
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	//�ͷ�������
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //IR_MJ_POWER��ǲ����

#if DBG
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	//ͨ����ǰջ�õ�MinorFunction
	ULONG fcn = stack->MinorFunction;

	//����IRP����
	static char* fcnname[] =
	{
		"IRP_MN_WAIT_WAKE",
		"IRP_MN_POWER_SEQUENCE",
		"IRP_MN_SET_POWER",
		"IRP_MN_QUERY_POWER",
	};


	if (fcn == IRP_MN_SET_POWER || fcn == IRP_MN_QUERY_POWER)
	{
	     //����ϵͳ��Դ״̬
		static char* sysstate[] =
		{
			"PowerSystemUnspecified",
			"PowerSystemWorking",
			"PowerSystemSleeping1",
			"PowerSystemSleeping2",
			"PowerSystemSleeping3",
			"PowerSystemHibernate",
			"PowerSystemShutdown",
			"PowerSystemMaximum",
		};

        //�����豸��Դ״̬
		static char* devstate[] =
		{
			"PowerDeviceUnspecified",
			"PowerDeviceD0",
			"PowerDeviceD1",
			"PowerDeviceD2",
			"PowerDeviceD3",
			"PowerDeviceMaximum",
		};

		ULONG context = stack->Parameters.Power.SystemContext;

		//�õ��豸��Դ״̬
		POWER_STATE_TYPE type = stack->Parameters.Power.Type;

		DbgPrint(DRIVERNAME " - IRP_MJ_POWER (%s)", fcnname[fcn]);

		//ͨ��type���ʹ�ӡ��ͬ��Ϣ
		if (type == SystemPowerState)
			DbgPrint(", SystemPowerState = %s\n", sysstate[stack->Parameters.Power.State.SystemState]);
		else
			DbgPrint(", DevicePowerState = %s\n", devstate[stack->Parameters.Power.State.DeviceState]);
	}

	else if (fcn < arraysize(fcnname))
		DbgPrint(DRIVERNAME " - IRP_MJ_POWER (%s)\n", fcnname[fcn]);

	else
		DbgPrint(DRIVERNAME " - IRP_MJ_POWER (%2.2X)\n", fcn);
#endif

    //�õ��豸��չ
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	//������һ����ԴIRP
	PoStartNextPowerIrp(Irp);
	NTSTATUS status;

	//��ȡ������
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//��ȡʧ��
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

    //�Թ���ǰ��ջ
	IoSkipCurrentIrpStackLocation(Irp);

	//�����²�����
	status = PoCallDriver(pdx->LowerDeviceObject, Irp);

	//�ͷ�������
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //IR_MJ_PNP��ǲ����

    //�õ���ǰ��ջ
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;
	NTSTATUS status;

	//�õ��豸��չ
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	//����������
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//����ʧ��
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

#if DBG
	static char* pnpname[] =         //�ܴ����IRP
	{
		"IRP_MN_START_DEVICE",
		"IRP_MN_QUERY_REMOVE_DEVICE",
		"IRP_MN_REMOVE_DEVICE",
		"IRP_MN_CANCEL_REMOVE_DEVICE",
		"IRP_MN_STOP_DEVICE",
		"IRP_MN_QUERY_STOP_DEVICE",
		"IRP_MN_CANCEL_STOP_DEVICE",
		"IRP_MN_QUERY_DEVICE_RELATIONS",
		"IRP_MN_QUERY_INTERFACE",
		"IRP_MN_QUERY_CAPABILITIES",
		"IRP_MN_QUERY_RESOURCES",
		"IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
		"IRP_MN_QUERY_DEVICE_TEXT",
		"IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
		"",
		"IRP_MN_READ_CONFIG",
		"IRP_MN_WRITE_CONFIG",
		"IRP_MN_EJECT",
		"IRP_MN_SET_LOCK",
		"IRP_MN_QUERY_ID",
		"IRP_MN_QUERY_PNP_DEVICE_STATE",
		"IRP_MN_QUERY_BUS_INFORMATION",
		"IRP_MN_DEVICE_USAGE_NOTIFICATION",
		"IRP_MN_SURPRISE_REMOVAL",
		"IRP_MN_QUERY_LEGACY_BUS_INFORMATION",
	};

	if (fcn < arraysize(pnpname))
		DbgPrint(DRIVERNAME " - IRP_MJ_PNP (%s)\n", pnpname[fcn]);
	else
		DbgPrint(DRIVERNAME " - IRP_MJ_PNP (%2.2X)\n", fcn);
#endif

    //����IRP_MN_DEVICE_USAGE_NOTIFICATION
	if (fcn == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		if (!fido->AttachedDevice || (fido->AttachedDevice->Flags & DO_POWER_PAGABLE))
			fido->Flags |= DO_POWER_PAGABLE;

        //������ǰ��ջ
		IoCopyCurrentIrpStackLocationToNext(Irp);

		//IO������̣����������̽����ڵ��ô˺�������������һ���������IRPָ���Ĳ�������ʱ������
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) UsageNotificationCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}

	// ����IRP_MN_START_DEVICE
	if (fcn == IRP_MN_START_DEVICE)
	{
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) StartDeviceCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}

	// ����IRP_MN_REMOVE_DEVICE
	if (fcn == IRP_MN_REMOVE_DEVICE)
	{
		IoSkipCurrentIrpStackLocation(Irp);

		//û���κδ���ֱ�����´���IRP
		status = IoCallDriver(pdx->LowerDeviceObject, Irp);

		//�ͷ���DispatchPnp�����л�ȡ��ɾ����
		IoReleaseRemoveLockAndWait(&pdx->RemoveLock, Irp);

        //�Ƴ������豸
		RemoveDevice(fido);
		return status;
	}

	// �Թ���ǰ��ջ
	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	//�ͷ�ɾ�����Լ�Ŀǰδ�����ɾ������
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}

///////////////////////////////////////////////////////////////////////////////
#pragma PAGEDCODE
ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo)
{
    //�õ���һ���ϵ͵��豸���豸��������͵ĺ���

    //��ȡ�豸����ָ�������
	PDEVICE_OBJECT ldo = IoGetAttachedDeviceReference(pdo);

	//��ȡ�豸ʧ��
	if (!ldo)
		return FILE_DEVICE_UNKNOWN;

	ULONG devtype = ldo->DeviceType;

	//����һ���豸��������ü���
	ObDereferenceObject(ldo);
	return devtype;
}

///////////////////////////////////////////////////////////////////////////////
#pragma PAGEDCODE
VOID RemoveDevice(IN PDEVICE_OBJECT fido)
{
    //ɾ���豸������

	DbgPrint("Enter RemoveDevice");
	PAGED_CODE();

	//ָ���豸��չ����
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	if (pdx->LowerDeviceObject)
		IoDetachDevice(pdx->LowerDeviceObject);

    //ɾ��fido
	IoDeleteDevice(fido);
}

///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
{
    //���ϴ���û�н��״̬�ĺ���

    //������ĳһ�㴦�������pengding״̬��
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	if (pdx->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		fido->Characteristics |= FILE_REMOVABLE_MEDIA;

    //�ͷ�������
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
{
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

    //����²����������flag��־��������Ҫͬ������
	if (!(pdx->LowerDeviceObject->Flags & DO_POWER_PAGABLE))
		fido->Flags &= ~DO_POWER_PAGABLE;

    //�ͷ�������
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
}

#pragma LOCKEDCODE
