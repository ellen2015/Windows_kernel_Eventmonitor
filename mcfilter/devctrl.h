#ifndef _DEVCTRL_H
#define _DEVCTRL_H

extern HANDLE g_deviceHandle;

typedef enum _NF_DRIVER_TYPE
{
	DT_UNKNOWN = 0,
	DT_TDI = 1,
	DT_WFP = 2
} NF_DRIVER_TYPE;

/**
*	Internal IO structure
**/
typedef struct _NF_DATA
{
	int				code;
	int				id;
	unsigned long	bufferSize;
	char 			buffer[1];
} NF_DATA, * PNF_DATA;

typedef  struct _NF_BUFFERS
{
	unsigned __int64 inBuf;
	unsigned __int64 inBufLen;
	unsigned __int64 outBuf;
	unsigned __int64 outBufLen;
} NF_BUFFERS, * PNF_BUFFERS;

typedef  struct _NF_READ_RESULT
{
	unsigned __int64 length;
} NF_READ_RESULT, * PNF_READ_RESULT;

class DevctrlIoct
{
public:
	DevctrlIoct();
	~DevctrlIoct();

	int devctrl_init();
	int devctrl_opendeviceSylink(const char* devSylinkName);
	int devctrl_workthread();
	int devctrl_waitSingeObject();
	void devctrl_clean();
	int devctrl_InitshareMem();
	void nf_setEventHandler(PVOID64 pHandler);
	int devctrl_OnMonitor();

private:
	HANDLE m_devhandler;
	HANDLE m_threadobjhandler;
	HANDLE m_alpcthreadobjhandler;
	HANDLE m_listthreadobjhandler;
	DWORD  m_dwthreadid;
	DWORD  m_dwthreadid1;

	// ���Ϳ�����
	int devctrl_sendioct(const int ioctcode);
	int devctrl_writeio();
	PVOID get_eventhandler();

};

#endif // !_DEVCTRL_H