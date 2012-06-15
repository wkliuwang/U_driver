#include "stddcls.h"
#include "Driver.h"
#include <srb.h>
#include <scsi.h>

//添加设备函数
NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo);

//设备卸载函数
VOID DriverUnload(IN PDRIVER_OBJECT fido);

//其他项派遣函数
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp);

//IRP_MJ_POWER派遣函数
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp);

//IRP_MJ_PNP派遣函数
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp);
NTSTATUS DispatchWmi(IN PDEVICE_OBJECT fido, IN PIRP Irp);

//得到设备类型
ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo);

//向上传递没有解决状态的函数，处理相应文件标志
NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);

//向上传递没有解决状态的函数，处理相应flag标志
NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx);



///////////////////////////////////////////////////////////////////////////////
#pragma INITCODE
extern "C" NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
	IN PUNICODE_STRING RegistryPath)
{
	DbgPrint(DRIVERNAME " - Entering DriverEntry: DriverObject %8.8lX\n", DriverObject);

	//初始化派遣函数
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
    //使用分页内存
	PAGED_CODE();
	DbgPrint(DRIVERNAME " - Entering DriverUnload: DriverObject %8.8lX\n", DriverObject);
}

///////////////////////////////////////////////////////////////////////////////

NTSTATUS AddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT pdo)
{
    //确保当前函数运行在分页内存
	PAGED_CODE();
	NTSTATUS status;

	PDEVICE_OBJECT fido;

	//创建设备对象
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL,
		GetDeviceTypeToUse(pdo), 0, FALSE, &fido);

    //判断是否成功创建设备对象
	if (!NT_SUCCESS(status))
	{
		DbgPrint(DRIVERNAME " - IoCreateDevice failed - %X\n", status);
		//如果不能成功创建设备对象则返回
		return status;
	}

	//获取设备扩展
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	do
	{
	    //初始化自旋锁
		IoInitializeRemoveLock(&pdx->RemoveLock, 0, 0, 0);
		pdx->DeviceObject = fido;
		pdx->Pdo = pdo;

		//将过滤驱动附加在底层驱动之上
		PDEVICE_OBJECT fdo = IoAttachDeviceToDeviceStack(fido, pdo);

		//如果附加操作失败
		if (!fdo)
		{
			DbgPrint(DRIVERNAME " - IoAttachDeviceToDeviceStack failed\n");
			status = STATUS_DEVICE_REMOVED;
			break;
		}

		//记录底层驱动
		pdx->LowerDeviceObject = fdo;

		//由于不知道底层驱动是直接IO还是BufferIO，因此将标志都置上
		fido->Flags |= fdo->Flags & (DO_DIRECT_IO | DO_BUFFERED_IO | DO_POWER_PAGABLE);

		//清除初始化标志以至于能够获取IRP
		fido->Flags &= ~DO_DEVICE_INITIALIZING;
	}	while (FALSE);

	//如果没有成功
	if (!NT_SUCCESS(status))
	{
	    //则从设备栈中删除设备
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
    //处理完请求函数

    //赋值相应参数调用完成函数即可
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
    //SCSI的完成例程，在这里修改底层驱动所做的处理，即实现U盘设备写保护

    //获取设备扩展
    PDEVICE_EXTENSION pdx = ( PDEVICE_EXTENSION )
                                   DeviceObject->DeviceExtension;

    //获取自旋锁
	IoAcquireRemoveLock(&pdx->RemoveLock,Irp);

    //获取当前IO堆栈
    PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation( Irp );

    //获取当前IRP处理状态
	PSCSI_REQUEST_BLOCK CurSrb=irpStack->Parameters.Scsi.Srb;
	PCDB cdb = (PCDB)CurSrb->Cdb;

	//获取操作码
	UCHAR opCode=cdb->CDB6GENERIC.OperationCode;

    //判断是否是SCSIOP_MODE_SENSE操作
	if(opCode==SCSIOP_MODE_SENSE  && CurSrb->DataBuffer
		&& CurSrb->DataTransferLength >=
		sizeof(MODE_PARAMETER_HEADER))
	{
	    //在SCSIOP_MODE_SENSE请求时修改变为只读
		DbgPrint("SCSIOP_MODE_SENSE comming!\n");

        //得到模式参数头
		PMODE_PARAMETER_HEADER modeData = (PMODE_PARAMETER_HEADER)CurSrb->DataBuffer;

        //设置U盘为只读状态！！！
		modeData->DeviceSpecificParameter |= MODE_DSP_WRITE_PROTECT;
	}

    //判断是否需要挂起
	if ( Irp->PendingReturned )
	{
		IoMarkIrpPending( Irp );
	}

    //释放自旋锁
	IoReleaseRemoveLock(&pdx->RemoveLock,Irp);

	return Irp->IoStatus.Status ;
}

#pragma LOCKEDCODE
NTSTATUS DispatchForSCSI(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //IRP_MJ_SCSI的派遣函数,首先将IRP发送到底层驱动，
    //然后设置完成例程，在完成例程中修改底层驱动所做的处理

    //获取设备扩展
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

    //获取当前IO堆栈
	PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);

	//向下层设备发送IRP
	NTSTATUS status;

	//获取自旋锁
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//获取失败
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

    //将当前IO堆栈复制到下层IO堆栈
	IoCopyCurrentIrpStackLocationToNext(Irp);

    //设置完成例程
	IoSetCompletionRoutine( Irp,
							USBSCSICompletion,
							NULL,
							TRUE,
							TRUE,
							TRUE );

    //调用底层设备对象行
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	//释放自旋锁
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}


///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS DispatchAny(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //其他派遣函数
    //对不做处理的IRP只需要将其直接穿过本层驱动即可

    //获取设备扩展
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	//获取IO堆栈
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
	};//定义IRP名字数组

    //获取IRP的主代码
	UCHAR type = stack->MajorFunction;
#endif

	NTSTATUS status;

	//获取自旋锁
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//是否成功获取
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

    //略过当前IO堆栈
	IoSkipCurrentIrpStackLocation(Irp);

	//调用底层驱动程序
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	//释放自旋锁
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchPower(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //IR_MJ_POWER派遣函数

#if DBG
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

	//通过当前栈得到MinorFunction
	ULONG fcn = stack->MinorFunction;

	//声明IRP类型
	static char* fcnname[] =
	{
		"IRP_MN_WAIT_WAKE",
		"IRP_MN_POWER_SEQUENCE",
		"IRP_MN_SET_POWER",
		"IRP_MN_QUERY_POWER",
	};


	if (fcn == IRP_MN_SET_POWER || fcn == IRP_MN_QUERY_POWER)
	{
	     //定义系统电源状态
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

        //定义设备电源状态
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

		//得到设备电源状态
		POWER_STATE_TYPE type = stack->Parameters.Power.Type;

		DbgPrint(DRIVERNAME " - IRP_MJ_POWER (%s)", fcnname[fcn]);

		//通过type类型打印不同信息
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

    //得到设备扩展
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	//处理下一个电源IRP
	PoStartNextPowerIrp(Irp);
	NTSTATUS status;

	//获取自旋锁
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//获取失败
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

    //略过当前堆栈
	IoSkipCurrentIrpStackLocation(Irp);

	//调用下层驱动
	status = PoCallDriver(pdx->LowerDeviceObject, Irp);

	//释放自旋锁
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}


///////////////////////////////////////////////////////////////////////////////
NTSTATUS DispatchPnp(IN PDEVICE_OBJECT fido, IN PIRP Irp)
{
    //IR_MJ_PNP派遣函数

    //得到当前堆栈
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	ULONG fcn = stack->MinorFunction;
	NTSTATUS status;

	//得到设备扩展
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	//声明自旋锁
	status = IoAcquireRemoveLock(&pdx->RemoveLock, Irp);

	//声明失败
	if (!NT_SUCCESS(status))
		return CompleteRequest(Irp, status, 0);

#if DBG
	static char* pnpname[] =         //能处理的IRP
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

    //处理IRP_MN_DEVICE_USAGE_NOTIFICATION
	if (fcn == IRP_MN_DEVICE_USAGE_NOTIFICATION)
	{
		if (!fido->AttachedDevice || (fido->AttachedDevice->Flags & DO_POWER_PAGABLE))
			fido->Flags |= DO_POWER_PAGABLE;

        //拷贝当前堆栈
		IoCopyCurrentIrpStackLocationToNext(Irp);

		//IO完成例程，这个完成例程将会在调用此函数的驱动的下一层驱动完成IRP指定的操作请求时被调用
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) UsageNotificationCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}

	// 处理IRP_MN_START_DEVICE
	if (fcn == IRP_MN_START_DEVICE)
	{
		IoCopyCurrentIrpStackLocationToNext(Irp);
		IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) StartDeviceCompletionRoutine,
			(PVOID) pdx, TRUE, TRUE, TRUE);
		return IoCallDriver(pdx->LowerDeviceObject, Irp);
	}

	// 处理IRP_MN_REMOVE_DEVICE
	if (fcn == IRP_MN_REMOVE_DEVICE)
	{
		IoSkipCurrentIrpStackLocation(Irp);

		//没有任何处理，直接向下传递IRP
		status = IoCallDriver(pdx->LowerDeviceObject, Irp);

		//释放在DispatchPnp例程中获取的删除锁
		IoReleaseRemoveLockAndWait(&pdx->RemoveLock, Irp);

        //移除功能设备
		RemoveDevice(fido);
		return status;
	}

	// 略过当前堆栈
	IoSkipCurrentIrpStackLocation(Irp);
	status = IoCallDriver(pdx->LowerDeviceObject, Irp);

	//释放删除锁以及目前未处理的删除操作
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return status;
}

///////////////////////////////////////////////////////////////////////////////
#pragma PAGEDCODE
ULONG GetDeviceTypeToUse(PDEVICE_OBJECT pdo)
{
    //得到设备对象的类型的函数，在创建设备对象时候调用

    //获取设备对象指针的引用
	PDEVICE_OBJECT ldo = IoGetAttachedDeviceReference(pdo);

	//获取设备失败
	if (!ldo)
		return FILE_DEVICE_UNKNOWN;

	ULONG devtype = ldo->DeviceType;

	//减少一个设备对象的引用计数
	ObDereferenceObject(ldo);
	return devtype;
}

///////////////////////////////////////////////////////////////////////////////
#pragma PAGEDCODE
VOID RemoveDevice(IN PDEVICE_OBJECT fido)
{
    //删除设备对象函数

	DbgPrint("Enter RemoveDevice");
	PAGED_CODE();

	//指向设备扩展对象
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION) fido->DeviceExtension;

	if (pdx->LowerDeviceObject)
		IoDetachDevice(pdx->LowerDeviceObject);

    //删除fido
	IoDeleteDevice(fido);
}

///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS StartDeviceCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
{
    //向上传递没有解决状态的函数,处理相应文件标志,IO完成例程中调用

    //在驱动某一层处理完后标记pengding状态。
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

	if (pdx->LowerDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
		fido->Characteristics |= FILE_REMOVABLE_MEDIA;

    //释放自旋锁
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
#pragma LOCKEDCODE
NTSTATUS UsageNotificationCompletionRoutine(PDEVICE_OBJECT fido, PIRP Irp, PDEVICE_EXTENSION pdx)
{
    ////向上传递没有解决状态的函数，处理相应flag标志,IO完成例程中调用

	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);

    //如果下层驱动清除了flag标志，这里需要同样处理
    //该DO_POWER_PAGABLE标志设置为0，则电源管理器可以在DISPATCH_LEVEL级上向你发送电源管理请求
	if (!(pdx->LowerDeviceObject->Flags & DO_POWER_PAGABLE))
		fido->Flags &= ~DO_POWER_PAGABLE;

    //释放自旋锁
	IoReleaseRemoveLock(&pdx->RemoveLock, Irp);
	return STATUS_SUCCESS;
}

#pragma LOCKEDCODE
