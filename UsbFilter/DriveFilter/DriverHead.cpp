#pragma once

#ifdef __cplusplus
extern "C"
{
#endif
#include <ntddk.h>
#ifdef __cplusplus
}
#endif

#define USBSTOR 	L"\\Registry\\Machine\\System\\CurrentControlSet\\Enum\\USBSTOR\\"
#define IOCTL_SETKEY CTL_CODE(\
	FILE_DEVICE_UNKNOWN,\
	0x801,\
	METHOD_BUFFERED,\
	FILE_ANY_ACCESS)


#define INITCODE code_seg("INIT")
#define PAGECODE code_seg("PAGE")

//���ļ�ΪWDM�ļ���ͷ�ļ�


typedef struct _DEVICE_EXTENSION{
	PDEVICE_OBJECT pDevice;

	//�豸��
	UNICODE_STRING ustrDeviceName;

	//��������
	UNICODE_STRING ustrSymLinkName;
}DEVICE_EXTENSION,*PDEVICE_EXTENSION;

typedef struct _DEVICE_PARAM{
	PWCHAR szKeyName;
	ULONG uFlag;
}DEVICE_PARAM,*PDEVICE_PARAM;


NTSTATUS CreateDevice(
								 IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

	UNICODE_STRING szDevName;
	RtlInitUnicodeString(&szDevName,L"\\Device\\ZdgDevice");

	//�����豸
	status=IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&(UNICODE_STRING)szDevName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&pDevObj);

    //�������ɹ��Ļ�
	if(!NT_SUCCESS(status))
	{
		DbgPrint("IoCreateDevice UnOK!\n");
		return status;
	}
	else
		DbgPrint("IoCreateDevice OK!\n");

    //��ֵ��Ӧ����
	pDevObj->Flags|=DO_BUFFERED_IO;
	pDevExt=(PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice=pDevObj;
	pDevExt->ustrDeviceName=szDevName;

	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName,L"\\??\\InstFilters");
	pDevExt->ustrSymLinkName=symLinkName;

    //������������
	status=IoCreateSymbolicLink(&symLinkName,&szDevName);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("IoCreateSymbolicLink UnOK");
		IoDeleteDevice(pDevObj);
		return status;
	}
	else
		DbgPrint("IoCreateSymbolicLink OK");

	return STATUS_SUCCESS;
}

extern "C" VOID DriverUnload(IN PDRIVER_OBJECT pDriverObject)
{
    //�豸ж�غ���

	PDEVICE_OBJECT pNextObj;
	DbgPrint("Enter DriverUnload\n");

    //�õ��豸
	pNextObj=pDriverObject->DeviceObject;

	while(pNextObj!=NULL)
	{
	    //�õ��豸��չ
		PDEVICE_EXTENSION pDevExt=(PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//�õ��豸������������
		UNICODE_STRING pLinkName=pDevExt->ustrSymLinkName;

		//ͨ����������ɾ���豸
		IoDeleteSymbolicLink(&pLinkName);

		//ȡ��һ���豸����ѭ��ɾ��
		pNextObj=pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}

	DbgPrint("DriverUnload Complete!\n");
}


NTSTATUS  DDKDispatchRoutine(
							 IN PDEVICE_OBJECT pDeviceObj,
							 IN PIRP pIrp)
{
    //��ǲ������

	DbgPrint("Enter  DDKDispatchRoutine \n");

	//�õ���ǰ��ջ
	PIO_STACK_LOCATION stack=IoGetCurrentIrpStackLocation(pIrp);

	//�ܴ����IRP����
	static CHAR* irpname[]=
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
		"IRP_MJ_MJ_POWER",
		"IRP_MJ_SYSTEM_CONTROL",
		"IRP_MJ_DEVICE_CHANGE",
		"IRP_MJ_QUERY_QUOTA",
		"IRP_MJ_SET_QUOTA",
		"IRP_MJ_PNP"
	};

	UCHAR type=stack->MajorFunction;

	//���Ƿ�Χ����֪��IRP����
	if(type>=ARRAYSIZE(irpname))
		DbgPrint("UnKnown IRP,major type is %x\n",type);
	else
		DbgPrint("\t%s\n",irpname[type]);

    //��״̬��ֵ�ɹ�
	NTSTATUS status=STATUS_SUCCESS;
	pIrp->IoStatus.Status=status;
	pIrp->IoStatus.Information=0;

    //��������
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	DbgPrint("Leave DispatchRoutine\n");
	return status;

}


NTSTATUS SetLowerFilters(PUNICODE_STRING szUName,ULONG uFlag)
{
    //���õײ���˺�������ɶ�ע���Ĵ���

	UNICODE_STRING dst,src,ValueName;
	HANDLE hRegister,hSubKey;
	WCHAR dst_buf[256];

	//��ʼ��UNICODE_STRING�ַ���
	RtlInitEmptyUnicodeString(&dst,dst_buf,256*sizeof(WCHAR));
	RtlInitUnicodeString(&src,USBSTOR);
	RtlCopyUnicodeString(&dst,&src);
	RtlAppendUnicodeStringToString(&dst,szUName);

	OBJECT_ATTRIBUTES objectAttributes;

	//��ʼ��objectAttributes
	InitializeObjectAttributes(&objectAttributes,
		&dst,
		OBJ_CASE_INSENSITIVE,//�Դ�Сд����
		NULL,
		NULL );

	//��ע���
	NTSTATUS ntStatus = ZwOpenKey( &hRegister,
		KEY_ALL_ACCESS,
		&objectAttributes);

    //��ע���ɹ�
	if (NT_SUCCESS(ntStatus))
	{
		DbgPrint("Open register successfully\n");
	}

	RtlInitUnicodeString(&ValueName,L"LowerFilters");
	PWCHAR strValue=L"USBFilter";

	ULONG ulSize;

	//��һ�ε���ZwQueryKeyΪ�˻�ȡKEY_FULL_INFORMATION���ݵĳ���
	ZwQueryKey(hRegister,
		KeyFullInformation,
		NULL,
		0,
		&ulSize);

	PKEY_FULL_INFORMATION pfi =
		(PKEY_FULL_INFORMATION)
		ExAllocatePool(PagedPool,ulSize);

	//�ڶ��ε���ZwQueryKeyΪ�˻�ȡKEY_FULL_INFORMATION���ݵ�����
	ZwQueryKey(hRegister,
		KeyFullInformation,
		pfi,
		ulSize,
		&ulSize);

	for (ULONG i=0;i<pfi->SubKeys;i++)
	{
		//��һ�ε���ZwEnumerateKeyΪ�˻�ȡKEY_BASIC_INFORMATION���ݵĳ���
		ZwEnumerateKey(hRegister,
			i,
			KeyBasicInformation,
			NULL,
			0,
			&ulSize);

		PKEY_BASIC_INFORMATION pbi =
			(PKEY_BASIC_INFORMATION)
			ExAllocatePool(PagedPool,ulSize);

		//�ڶ��ε���ZwEnumerateKeyΪ�˻�ȡKEY_BASIC_INFORMATION���ݵ�����
		ZwEnumerateKey(hRegister,
			i,
			KeyBasicInformation,
			pbi,
			ulSize,
			&ulSize);

		UNICODE_STRING uniKeyName;
		uniKeyName.Length =
			uniKeyName.MaximumLength =
			(USHORT)pbi->NameLength;

		uniKeyName.Buffer = pbi->Name;

		WCHAR bufTmp[256];

		//��ʼ��ֵ
		RtlInitEmptyUnicodeString(&src,bufTmp,256*sizeof(WCHAR));
		RtlCopyUnicodeString(&src,&dst);

		UNICODE_STRING szX;
		RtlInitUnicodeString(&szX,L"\\");
		RtlAppendUnicodeStringToString(&src,&szX),
			RtlAppendUnicodeStringToString(&src,&uniKeyName);

        //��ʼ��ObjectAttributes
		InitializeObjectAttributes(&objectAttributes,
			&src,
			OBJ_CASE_INSENSITIVE,//�Դ�Сд����
			NULL,
			NULL );

        //��ע���
		ZwOpenKey( &hSubKey,
			KEY_ALL_ACCESS,
			&objectAttributes);

        //uFlagΪ0��ɾ��ע����ֵ����������ע����ֵ
		if(uFlag==0)
		{
			ntStatus=ZwDeleteValueKey(hSubKey,&ValueName);
		}
		else
		{
		ntStatus=ZwSetValueKey(hSubKey,&ValueName,0,REG_SZ,strValue,wcslen(strValue)*2+2);
		}

		//�ɹ��Ļ�
		if (NT_SUCCESS(ntStatus))
		{
			DbgPrint("Charge Value successfully\n");
		}
		else
			DbgPrint("Charge Value failed!\n");

		DbgPrint("The %d sub item name:%wZ\n",i,&src);

        //�����ڴ�
		ExFreePool(pbi);

		//�رվ��
		ZwClose(hSubKey);
	}

	ExFreePool(pfi);
	ZwClose(hRegister);

	return STATUS_SUCCESS;
}


NTSTATUS DDKDeviceIoControl(IN PDEVICE_OBJECT pDevObj,
							IN PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint(("Enter DeviceIOControl\n"));

    //�õ���ǰ��ջ
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	//�õ����뻺������С
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;

	//�õ������������С
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;

	//�õ�IOCTL��
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	ULONG info = cbout;

	switch (code)
	{
	case   IOCTL_SETKEY:
		{
			DbgPrint(" IOCTL_SETKEY\n");

            //��ʾ���뻺��������
			PDEVICE_PARAM InputBuffer = (PDEVICE_PARAM)pIrp->AssociatedIrp.SystemBuffer;
			UNICODE_STRING szTmp;

			//��ʼ��szTmp
			RtlInitUnicodeString(&szTmp,InputBuffer->szKeyName);

            //���ú������õײ����
			SetLowerFilters(&szTmp,InputBuffer->uFlag);
			DbgPrint("%ws,%d",InputBuffer->szKeyName,InputBuffer->uFlag);
			info = cbout;
			break;
		}

	default:
		status = STATUS_INVALID_VARIANT;
	}

	// ���IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );

	UNICODE_STRING uniStr;
	RtlInitUnicodeString(&uniStr,L"Zdg Do it");

	DbgPrint("The output buffer is %wZ\n",uniStr);
	return status;
}

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(
								IN PDRIVER_OBJECT pDriverObject,
								IN PUNICODE_STRING pRegistryPath)
{
    //��ڳ���

    //����ж����ǲ����
	pDriverObject->DriverUnload=DriverUnload;

	DbgPrint("DriverEntry Started!\n");

    //������Ӧ��ǲ����
	pDriverObject->MajorFunction[IRP_MJ_CREATE]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]=DDKDispatchRoutine;
	//�����豸������ǲ����
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=DDKDeviceIoControl;

    //�����豸
	NTSTATUS status=CreateDevice(pDriverObject);

	DbgPrint("DriverEntry Ended!\n");
	return STATUS_SUCCESS;
}
