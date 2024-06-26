#pragma once

#include "ClientSocket.h"
#include "CWatchDialog.h"
#include "RemoteClientDlg.h"
#include "StatusDlg.h"
#include "resource.h"
#include "Tool.h"
#include <map>

#define WM_SEND_PACK (WM_USER + 1)		//�������ݰ�
#define WM_SEND_DATA (WM_USER + 2)		//��������
#define WM_SHOW_STATUE (WM_USER + 3)	//չʾ״̬
#define WM_SHOW_WATCH (WM_USER + 4)		//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER + 0X1000)	//�Զ�����Ϣ����


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
	int Invoke(CWnd*& pMainWnd);	//��Ϣ������

	//������Ϣ
	LRESULT SendMessage(MSG msg);

	//Control -> ����Socket ����ip��ַ�Ͷ˿ں�
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

	bool SendCommandPacket(
		HWND hWnd,	//���ݰ��յ��� ��ҪӦ��Ĵ���
		int nCmd, 
		bool bAutoClose = true, 
		BYTE* pData = NULL, 
		size_t nLength = 0,
		WPARAM wParam = 0)
	{
		TRACE("%s nCmd = %d, tick = %llu\r\n",
			__FUNCTION__, nCmd, GetTickCount64());

		bool ret = CClientSocket::getInstance().SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);//�ͻ��˷��������
		//if (!ret)
		//{
		//	Sleep(30);
		//	ret = CClientSocket::getInstance().SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
		//}
		return ret;
	}

	//int getImage(CImage& image)
	//{
	//	return Tool::Bytes2Image(image, CClientSocket::getInstance().getPacket().strData);
	//}
	void DownloadEnd()
	{
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
	}




	int downFile(CString strPath)
	{

		CFileDialog dlg(FALSE, NULL, strPath,
			OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY, NULL, &m_remoteDlg);

		if (dlg.DoModal() == IDOK)
		{
			m_strRemote = strPath;
			m_strLocal = dlg.GetPathName();

			FILE* pFile = fopen(m_strLocal, "wb+");
			if (pFile == NULL)
			{
				AfxMessageBox("û��Ȩ�ޱ�����ļ��� �����ļ��޷�����������");
				return -1;
			}

			int ret = SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
			//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
			//
			//if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
			//{
			//	return -1;
			//}


			m_remoteDlg.BeginWaitCursor();
			m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));
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

//�����ļ�
protected:
	void threadDownloadFile();
	static void __stdcall threadDownloadEntry(void* arg);

//��Ƶ���
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



	CString m_strRemote;	//�����ļ���Զ��·��
	CString m_strLocal;		//�����ļ��ı��ر���·��


	//Զ�̼�ر���
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

