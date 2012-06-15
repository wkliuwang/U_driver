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

//InstFilters作用就是在注册表上，根据UsbLocker选中的U盘型号，在相应U盘型号目录下
//建立注册表LowerFilters子键，这样当该U盘被插入时候，操作系统就会加载UsbFilter
//的过滤驱动程序


typedef struct _DEVICE_EXTENSION{
	PDEVICE_OBJECT pDevice;

	//设备名
	UNICODE_STRING ustrDeviceName;

	//符号链接
	UNICODE_STRING ustrSymLinkName;
}DEVICE_EXTENSION,*PDEVICE_EXTENSION;

typedef struct _DEVICE_PARAM{
    //记录选中U盘的型号
	PWCHAR szKeyName;

	//标识位，1表示开写保护，0表示关写保护
	ULONG uFlag;
}DEVICE_PARAM,*PDEVICE_PARAM;

//初始化设备对象
NTSTATUS CreateDevice(IN PDRIVER_OBJECT pDriverObject)
{
	NTSTATUS status;
	PDEVICE_OBJECT pDevObj;
	PDEVICE_EXTENSION pDevExt;

    //创建设备名称
	UNICODE_STRING szDevName;
	RtlInitUnicodeString(&szDevName,L"\\Device\\ZdgDevice");

	//创建设备
	status=IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&(UNICODE_STRING)szDevName,
		FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,
		TRUE,
		&pDevObj);

    //创建不成功的话
	if(!NT_SUCCESS(status))
	{
		DbgPrint("IoCreateDevice UnOK!\n");
		return status;
	}
	else
		DbgPrint("IoCreateDevice OK!\n");

    //赋值相应参数
	pDevObj->Flags|=DO_BUFFERED_IO;
	pDevExt=(PDEVICE_EXTENSION)pDevObj->DeviceExtension;
	pDevExt->pDevice=pDevObj;
	pDevExt->ustrDeviceName=szDevName;

    //创建符号链接
	UNICODE_STRING symLinkName;
	RtlInitUnicodeString(&symLinkName,L"\\??\\InstFilters");
	pDevExt->ustrSymLinkName=symLinkName;

    //创建符号链接
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
    //设备卸载函数

	PDEVICE_OBJECT pNextObj;
	DbgPrint("Enter DriverUnload\n");

    //得到设备
	pNextObj=pDriverObject->DeviceObject;

	while(pNextObj!=NULL)
	{
	    //得到设备扩展
		PDEVICE_EXTENSION pDevExt=(PDEVICE_EXTENSION)pNextObj->DeviceExtension;

		//得到设备符号链接名称
		UNICODE_STRING pLinkName=pDevExt->ustrSymLinkName;

		//通过符号链接删除设备
		IoDeleteSymbolicLink(&pLinkName);

		//取下一个设备继续循环删除
		pNextObj=pNextObj->NextDevice;
		IoDeleteDevice(pDevExt->pDevice);
	}

	DbgPrint("DriverUnload Complete!\n");
}


NTSTATUS  DDKDispatchRoutine(
							 IN PDEVICE_OBJECT pDeviceObj,
							 IN PIRP pIrp)
{
    //派遣处理函数

	DbgPrint("Enter  DDKDispatchRoutine \n");

	//得到当前堆栈
	PIO_STACK_LOCATION stack=IoGetCurrentIrpStackLocation(pIrp);

	//能处理的IRP类型
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

	//不是范围内已知的IRP类型
	if(type>=ARRAYSIZE(irpname))
		DbgPrint("UnKnown IRP,major type is %x\n",type);
	else
		DbgPrint("\t%s\n",irpname[type]);

    //将状态赋值成功
	NTSTATUS status=STATUS_SUCCESS;
	pIrp->IoStatus.Status=status;
	pIrp->IoStatus.Information=0;

    //结束请求
	IoCompleteRequest(pIrp,IO_NO_INCREMENT);
	DbgPrint("Leave DispatchRoutine\n");
	return status;

}


 //在注册表子项中设置LowerFilters子项
NTSTATUS SetLowerFilters(PUNICODE_STRING szUName,ULONG uFlag)
{
	UNICODE_STRING dst,src,ValueName;
	HANDLE hRegister,hSubKey;
	WCHAR dst_buf[256];

	//初始化UNICODE_STRING字符串
	RtlInitEmptyUnicodeString(&dst,dst_buf,256*sizeof(WCHAR));
	//初始化src为U盘注册表位置
	RtlInitUnicodeString(&src,USBSTOR);
	RtlCopyUnicodeString(&dst,&src);

	//将U盘名字追加到注册表位置末尾，为选中U盘的实际注册表位置
	RtlAppendUnicodeStringToString(&dst,szUName);

	OBJECT_ATTRIBUTES objectAttributes;

	//初始化objectAttributes
	InitializeObjectAttributes(&objectAttributes,
		&dst,               //文件名
		OBJ_CASE_INSENSITIVE,//对大小写敏感
		NULL,
		NULL );

	//打开注册表
	NTSTATUS ntStatus = ZwOpenKey( &hRegister,   //返回被打开的句柄
		KEY_ALL_ACCESS,         //打开权限，一般都这么设置
		&objectAttributes);

    //打开注册表成功
	if (NT_SUCCESS(ntStatus))
	{
		DbgPrint("Open register successfully\n");
	}

    //要在注册表子项下添加LowerFilters的表项
	RtlInitUnicodeString(&ValueName,L"LowerFilters");

	//键值设为过滤类名，如果随便设一个会导致U盘连接状态但是不能查看到
	PWCHAR strValue=L"UsbFilter";

	ULONG ulSize;

	//第一次调用ZwQueryKey为了获取KEY_FULL_INFORMATION数据的长度(因为是变长的)
	ZwQueryKey(hRegister,
		KeyFullInformation,  //查询类别，一般都这么设置
		NULL,
		0,
		&ulSize);         //返回数据KEY_FULL_INFORMATION的长度

	PKEY_FULL_INFORMATION pfi =
		(PKEY_FULL_INFORMATION)
		ExAllocatePool(PagedPool,ulSize);

	//第二次调用ZwQueryKey为了获取KEY_FULL_INFORMATION数据的数据
	ZwQueryKey(hRegister,
		KeyFullInformation,
		pfi,      //查询数据的指针
		ulSize,   //数据长度
		&ulSize);

    //ZwQueryKey的作用主要就是获得某注册表究竟有多少个子项
    //而ZwEnumerateKey的作用主要是针对第几个子项获取该子项的具体信息

	for (ULONG i=0;i<pfi->SubKeys;i++)
	{
		//第一次调用ZwEnumerateKey为了获取KEY_BASIC_INFORMATION数据的长度
		ZwEnumerateKey(hRegister,
			i,
			KeyBasicInformation,
			NULL,
			0,
			&ulSize);

		PKEY_BASIC_INFORMATION pbi =
			(PKEY_BASIC_INFORMATION)
			ExAllocatePool(PagedPool,ulSize);

		//第二次调用ZwEnumerateKey为了获取KEY_BASIC_INFORMATION数据的数据
		ZwEnumerateKey(hRegister,
			i,
			KeyBasicInformation,
			pbi,
			ulSize,
			&ulSize);

		UNICODE_STRING uniKeyName;

		//获取子项名字的长度
		uniKeyName.Length =
			uniKeyName.MaximumLength =
			(USHORT)pbi->NameLength;

		uniKeyName.Buffer = pbi->Name;

		WCHAR bufTmp[256];

		//初始化值
		RtlInitEmptyUnicodeString(&src,bufTmp,256*sizeof(WCHAR));
		RtlCopyUnicodeString(&src,&dst);

		UNICODE_STRING szX;
		RtlInitUnicodeString(&szX,L"\\");
		RtlAppendUnicodeStringToString(&src,&szX),

		//这里就得到注册表子项名字的具体路径名，保存在src中
        RtlAppendUnicodeStringToString(&src,&uniKeyName);

        //初始化ObjectAttributes
		InitializeObjectAttributes(&objectAttributes,
			&src,
			OBJ_CASE_INSENSITIVE,//对大小写敏感
			NULL,
			NULL );

        //打开注册表
		ZwOpenKey( &hSubKey,
			KEY_ALL_ACCESS,
			&objectAttributes);

        //uFlag为0则删除注册表键值(既关写保护)，否则设置注册表键值(既开写保护)
		if(uFlag==0)
		{
			ntStatus=ZwDeleteValueKey(hSubKey,&ValueName);
		}
		else
		{
		ntStatus=ZwSetValueKey(hSubKey,&ValueName,0,REG_SZ,strValue,wcslen(strValue)*2+2);
		}

		//成功的话
		if (NT_SUCCESS(ntStatus))
		{
			DbgPrint("Charge Value successfully\n");
		}
		else
			DbgPrint("Charge Value failed!\n");

		DbgPrint("The %d sub item name:%wZ\n",i,&src);

        //回收内存
		ExFreePool(pbi);

		//关闭句柄
		ZwClose(hSubKey);
	}

	ExFreePool(pfi);
	ZwClose(hRegister);

	return STATUS_SUCCESS;
}


//UsbLocker里调用DeviceIoControl函数时候，会调用该函数
NTSTATUS DDKDeviceIoControl(IN PDEVICE_OBJECT pDevObj,
							IN PIRP pIrp)
{
	NTSTATUS status = STATUS_SUCCESS;
	DbgPrint(("Enter DeviceIOControl\n"));

    //得到当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);

	//得到输入缓冲区大小
	ULONG cbin = stack->Parameters.DeviceIoControl.InputBufferLength;

	//得到输出缓冲区大小
	ULONG cbout = stack->Parameters.DeviceIoControl.OutputBufferLength;

	//得到IOCTL码
	ULONG code = stack->Parameters.DeviceIoControl.IoControlCode;

	ULONG info = cbout;

	switch (code)
	{
	case   IOCTL_SETKEY:
		{
			DbgPrint(" IOCTL_SETKEY\n");

            //显示输入缓冲区数据
			PDEVICE_PARAM InputBuffer = (PDEVICE_PARAM)pIrp->AssociatedIrp.SystemBuffer;
			UNICODE_STRING szTmp;

			//初始化szTmp,使之赋值为传递来的选中的U盘注册表的路径
			RtlInitUnicodeString(&szTmp,InputBuffer->szKeyName);

            //传递U盘注册表的路径，和标志，根据标志判断添加还是删除注册表
			SetLowerFilters(&szTmp,InputBuffer->uFlag);
			DbgPrint("%ws,%d",InputBuffer->szKeyName,InputBuffer->uFlag);
			info = cbout;
			break;
		}

	default:
		status = STATUS_INVALID_VARIANT;
	}

	// 完成IRP
	pIrp->IoStatus.Status = status;
	pIrp->IoStatus.Information = info;
	IoCompleteRequest( pIrp, IO_NO_INCREMENT );

	UNICODE_STRING uniStr;
	RtlInitUnicodeString(&uniStr,L"Done");

	DbgPrint("The output buffer is %wZ\n",uniStr);
	return status;
}

#pragma INITCODE
extern "C" NTSTATUS DriverEntry(
								IN PDRIVER_OBJECT pDriverObject,
								IN PUNICODE_STRING pRegistryPath)
{
    //入口程序

    //设置卸载派遣函数
	pDriverObject->DriverUnload=DriverUnload;

	DbgPrint("DriverEntry Started!\n");

    //设置相应派遣函数
	pDriverObject->MajorFunction[IRP_MJ_CREATE]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_CLEANUP]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_SET_INFORMATION]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_SHUTDOWN]=DDKDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL]=DDKDispatchRoutine;
	//设置设备控制派遣函数
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]=DDKDeviceIoControl;

    //创建设备
	NTSTATUS status=CreateDevice(pDriverObject);

	DbgPrint("DriverEntry Ended!\n");
	return STATUS_SUCCESS;
}
