#include "pch.h"
#include "ClientController.h"
  

std::map<INT, CClientController::MSGFUNC> 
CClientController::m_mapFunc;

CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;


CClientController::CClientController()
	: m_statusDlg(&m_remoteDlg),
	  m_watchDlg(&m_remoteDlg)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_hThreadDownload = INVALID_HANDLE_VALUE;
	m_nThreadID = -1;
	
	m_watchIsClose = true;
	
	MAPDATA data[] =
	{
		//{WM_SEND_PACK, &CClientController::OnSendPack},
		//{WM_SEND_DATA, &CClientController::OnSendData},
		{WM_SHOW_STATUE, &CClientController::OnShowStatus},
		{WM_SHOW_WATCH, &CClientController::OnShowWatch},
		{-1, nullptr}
	};
	for (int i = 0; data[i].nMsg != -1; i++)
	{
		m_mapFunc.insert(std::pair<INT, MSGFUNC>(data[i].nMsg, data[i].func));
	}
}


unsigned _stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}


LRESULT CClientController::SendMessage(MSG msg)
{
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	MSGINFO info(msg);

	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE, 
		(WPARAM)&info, (LPARAM)&hEvent);

	WaitForSingleObject(hEvent, INFINITE);

	CloseHandle(hEvent);
	
	return info.result;
}


void CClientController::threadFunc()
{

	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TRACE("GET MESSAGE!\r\n");
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE)
		{
			MSGINFO* pmsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<INT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);

			if (it != m_mapFunc.end())
			{
				pmsg->result = (this->*it->second)(pmsg->msg.message, 
				pmsg->msg.wParam, pmsg->msg.lParam);
			}
			else
			{
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else
		{
			std::map<INT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end())
			{
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}

	}
} 

bool CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0,
		&CClientController::threadEntry, this, 0, &m_nThreadID);
	
	if (m_hThread == 0)
	{
		TRACE("controller thread init failed!\r\n");
		return false;
	}
	m_statusDlg.Create(IDD_DIG_STATUS, &m_remoteDlg);

	return true;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

/*
LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CPacket* pPacket = (CPacket*)wParam;

	return CClientSocket::getInstance().Send(*pPacket);
}
*/

//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	char* pBuffer = (char*)wParam;
//	
//	return CClientSocket::getInstance().Send(pBuffer, (int)lParam);
//}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatch(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}


void CClientController::threadDownloadFile()
{
	FILE* pFile = fopen(m_strLocal, "wb+");
	if (pFile == NULL)
	{
		AfxMessageBox("没有权限保存该文件， 或者文件无法创建！！！");
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();
		return;
	}
	
	
	do {
		int ret = SendCommandPacket(4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength());
		//int ret = SendMessage(WM_SEND_PACKET, 4 << 1 | 0, (LPARAM)(LPCSTR)strFile);
		if (ret < 0)
		{
			AfxMessageBox("执行下载命令失败！");
			TRACE("%s(%d) 执行下载失败 ret = %d\r\n", __FUNCTION__, __LINE__, ret);
			return;
		}

		long long nLength = *(long long*)CClientSocket::getInstance().getPacket().strData.c_str();
		if (nLength == 0)
		{
			AfxMessageBox("文件长度为0 或者 无法读取文件！！！");
			m_statusDlg.ShowWindow(SW_HIDE);
			m_remoteDlg.EndWaitCursor();
			return;
		}
		long long nCount = 0;
		while (nCount < nLength)
		{
			CClientSocket::getInstance().dealCommand();
			if (ret < 0)
			{
				AfxMessageBox("传输失败！");
				TRACE("传输失败， ret = %d\r\n", ret);
				break;
			}
			fwrite(CClientSocket::getInstance().getPacket().strData.c_str(),
				1,
				CClientSocket::getInstance().getPacket().strData.size(),
				pFile);
			nCount += CClientSocket::getInstance().getPacket().strData.size();
		}
	} while (false);

	fclose(pFile);
	CClientSocket::getInstance().closeClient();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("下载完成！"), _T("完成"));
}

void __stdcall CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
}

void CClientController::threadWatchData()
{
	ULONGLONG tick = GetTickCount64();
	while (!m_watchIsClose)
	{
		if (m_watchDlg.isFull() == false)
		{
			std::list<CPacket> lstPackets;
			TRACE("&lstPackets = %p\r\n", &lstPackets);
			int ret = SendCommandPacket(6, true, NULL, 0, &lstPackets);
			//int ret = SendMessage(WM_SEND_PACKET, 6 << 1 | 0);
			if (ret == 6)
			{
				if (Tool::Bytes2Image(m_watchDlg.GetImage(), lstPackets.front().strData) == 0)
				{
					m_watchDlg.SetImageStatus(true);
				}
				else
				{
					TRACE("获取图片失败！ \r\n");
				}
			}
			else
			{
				Sleep(1);
			}
		}
	}
}

void __stdcall CClientController::threadEntryForWatchData(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchData();
	_endthread();
}

void CClientController::startWatch()
{
	m_watchIsClose = false;
	//m_watchDlg.SetParent(&m_remoteDlg);

	//CWatchDialog dlg(&m_remoteDlg);
	HANDLE hThread = (HANDLE)_beginthread(CClientController::threadEntryForWatchData, 0, this);
	m_watchDlg.DoModal();
	m_watchIsClose = true;
	DWORD dwReturn = WaitForSingleObject(hThread, 500);
}


