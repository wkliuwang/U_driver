// Declarations for filter driver

// Copyright (C) 1999 by Walter Oney

// All rights reserved



#ifndef DRIVER_H

#define DRIVER_H


//����Ϣ��ʹ��
#define DRIVERNAME "USBFilter(Zdg)"



///////////////////////////////////////////////////////////////////////////////


//Ŀ���豸��չ�ṹ
typedef struct tagDEVICE_EXTENSION {

    // ����豸���ڵ��豸����
	PDEVICE_OBJECT DeviceObject;

    // �ͼ�����������������ͬ��ջ��
	PDEVICE_OBJECT LowerDeviceObject;

    //�����豸����PDO
	PDEVICE_OBJECT Pdo;

    //ɾ����
	IO_REMOVE_LOCK RemoveLock;

	} DEVICE_EXTENSION, *PDEVICE_EXTENSION;



///////////////////////////////////////////////////////////////////////////////


//����ȫ�ֺ���

//ж���豸
VOID RemoveDevice(IN PDEVICE_OBJECT fdo);

//�������
NTSTATUS CompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info);

//IRP_MJ_SCSI����ǲ����s
NTSTATUS DispatchForSCSI(IN PDEVICE_OBJECT fido, IN PIRP Irp);

#endif // DRIVER_H

