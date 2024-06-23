#pragma once

#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "Tool.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)		//发送数据包
#define WM_SEND_DATA (WM_USER + 2)		//发送数据
#define WM_SHOW_STATUE (WM_USER + 3)	//展示状态
#define WM_SHOW_WATCH (WM_USER + 4)		//远程监控
#define WM_SEND_MESSAGE (WM_USER + 0X1000)	//自定义消息处理


class CClientController
{
public:
	//getInstance();
	//static CClientController& getInstance()
	//{
	//	static CClientController m_instance;
	//	return m_instance;
	//}
	static CClientController* getInstance()
	{
		if (m_instance == NULL)
		{
			m_instance = new CClientController();
		}
		return m_instance;
	}
	

	bool InitController();
	int Invoke(CWnd*& pMainWnd);	//消息泵启动

	//发送消息
	LRESULT SendMessage(MSG msg);

	//Control -> 控制Socket 更新ip地址和端口号
	void UpdateAddress(int nIP, int nPort)
	{
		CClientSocket::getInstance().UpdateAddress(nIP, nPort);
	}

	int dealCommand()
	{
		int cmd = CClientSocket::getInstance().dealCommand();
		return cmd;
	}

	int closeClient()
	{
		int cmd = CClientSocket::getInstance().closeClient();
		return cmd;
	}

	//void SendPacket(const CPacket& pack)
	//{
	//	if (CClientSocket::getInstance().initSocket() == true)
	//	{
	//		if (CClientSocket::getInstance().Send(pack) == false)
	//		{
	//			TRACE("control send packet failed!\r\n");
	//		}
	//	}
	//	else
	//	{
	//		TRACE("control init socket failed!\r\n");
	//	}
	//}

	int SendCommandPacket(
		int nCmd, 
		bool bAutoClose = true, 
		BYTE* pData = NULL, 
		size_t nLength = 0,
		std::list<CPacket>* plstPackets = NULL)
	{
		TRACE("%s nCmd = %d, tick = %llu\r\n",
			__FUNCTION__, nCmd, GetTickCount64());

		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		//SendPacket(CPacket(nCmd, pData, nLength, hEvent));
		
		std::list<CPacket> lstPackets;	// 服务器应答包
		if (plstPackets == NULL)
			plstPackets = &lstPackets;

		TRACE("&lstPackets = %p\r\n", plstPackets);
		CClientSocket::getInstance().SendPacket(CPacket(nCmd, pData, nLength, hEvent),
			*plstPackets, bAutoClose);//客户端发送命令包
		
		CloseHandle(hEvent);	//回收事件句柄，防止资源耗尽

		if (plstPackets->size() > 0)	//如果服务器回传了应答包
		{
			TRACE("%s nCmd = %d, tick = %llu\r\n",
				__FUNCTION__, nCmd, GetTickCount64());
			return plstPackets->front().sCmd;
		}
		return -1;
	}

	//int getImage(CImage& image)
	//{
	//	return Tool::Bytes2Image(image, CClientSocket::getInstance().getPacket().strData);
	//}


	int downFile(CString strPath)
	{

		CFileDialog dlg(FALSE, NULL, strPath,
			OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, NULL, &m_remoteDlg);

		if (dlg.DoModal() == IDOK)
		{
			m_strRemote = strPath;
			m_strLocal = dlg.GetPathName();

			m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
			
			if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
			{
				return -1;
			}


			m_remoteDlg.BeginWaitCursor();
			m_statusDlg.m_info.SetWindowText(_T("命令正在执行中！"));
			m_statusDlg.ShowWindow(SW_SHOW);
			m_statusDlg.CenterWindow(&m_remoteDlg);
			m_statusDlg.SetActiveWindow();
		}
		return 0;
		//_beginthread(CRemoteClientDlg::threadEntryForDownFile, 0, this);
	}

	void startWatch();

protected:
	CClientController();

	~CClientController()
	{
		TRACE("~CClientController() \r\n");
		WaitForSingleObject(m_hThread, 100);
		m_remoteDlg.DestroyWindow();
		m_statusDlg.DestroyWindow();
		m_watchDlg.DestroyWindow();
	}

	void threadFunc();
	static unsigned _stdcall threadEntry(void* arg);
	
	LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam);

//下载文件
protected:
	void threadDownloadFile();
	static void __stdcall threadDownloadEntry(void* arg);

//视频监控
protected:
	void threadWatchData();
	static void __stdcall threadEntryForWatchData(void* arg);



private:
	typedef struct MsgInfo {
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo(const MsgInfo& m)
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m)
		{
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	}MSGINFO;


	using MSGFUNC = 
		LRESULT(CClientController::*)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	using MAPDATA = 
	struct
	{
		INT	nMsg;
		MSGFUNC func;
	};

	static std::map<INT, MSGFUNC> m_mapFunc;

	//std::map<UUID, MSG> m_mapMessage;

	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;

	HANDLE m_hThread;
	HANDLE m_hThreadDownload;



	CString m_strRemote;	//下载文件的远程路径
	CString m_strLocal;		//下载文件的本地保存路径


	//远程监控变量
	bool   m_watchIsClose;

	unsigned m_nThreadID;

	static CClientController* m_instance;

	static void releaseInstance(void)
	{
		if (m_instance != NULL)
		{
			CClientController* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}

	class CHelper {
	public:
		CHelper()
		{
			//CClientController::getInstance();
		}
		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
};

