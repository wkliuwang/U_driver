// Declarations for filter driver

// Copyright (C) 1999 by Walter Oney

// All rights reserved



#ifndef DRIVER_H

#define DRIVER_H


//在信息中使用
#define DRIVERNAME "UsbFilter"



///////////////////////////////////////////////////////////////////////////////


//目标设备扩展结构
typedef struct tagDEVICE_EXTENSION {

    // 这个设备属于的设备对象
	PDEVICE_OBJECT DeviceObject;

    // 低级的驱动，本例在相同的栈上
	PDEVICE_OBJECT LowerDeviceObject;

    //物理设备对象PDO
	PDEVICE_OBJECT Pdo;

    //删除锁
	IO_REMOVE_LOCK RemoveLock;

	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



///////////////////////////////////////////////////////////////////////////////


//定义全局函数

//卸载设备
VOID RemoveDevice(IN PDEVICE_OBJECT fdo);

//完成请求
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);

//IRP_MJ_SCSI的派遣函数
NTSTATUS DispatchForSCSI(IN PDEVICE_OBJECT fido, IN PIRP Irp);

#endif // DRIVER_H

