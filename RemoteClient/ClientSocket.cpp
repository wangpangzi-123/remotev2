#include "pch.h"
#include "ClientSocket.h"

CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	m_socket = ss.m_socket;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;

	std::map<UINT, MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}



std::string GetErrInfo(int wsaErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;

	FormatMessage(	//错误码 格式化 参数
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}


unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

/*
void CClientSocket::threadFunc()
{

	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	if (initSocket() == false)
	{
		TRACE("init socket failed!\r\n");
	}
	while (m_socket != INVALID_SOCKET)
	{
		if (m_lstSend.size() > 0)
		{
			TRACE("lstSend size: %d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();


			if (Send(head) == false)
			{
				TRACE("发送失败 ！ \r\n");
				continue;
			}
			//auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>>(head.hEvent, std::list<CPacket>()));
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end())
			{
				std::map<HANDLE, bool>::iterator it0;
				it0 = m_mapAutoClosed.find(head.hEvent);
				TRACE("it0 -> second = %d\r\n", it0->second);
				do
				{
					int length = recv(m_socket, pBuffer + index, BUFFER_SIZE - index, 0);
					if ((length > 0) || (index > 0))
					{
						index += length;
						size_t size = index;
						CPacket pack((BYTE*)pBuffer, size);

						if (size > 0)
						{
							pack.hEvent = head.hEvent;
							TRACE("&lstPackets = %p\r\n", it->second);
							it->second.push_back(pack);

							memmove(pBuffer, pBuffer + size, index - size);
							index -= size;
							if (it0->second)
							{
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0 && index <= 0)
					{
						closeClient();
						SetEvent(head.hEvent);
						if (it0 != m_mapAutoClosed.end())
						{
							TRACE("Set Event %d %d\r\n", head.sCmd, it0->second);
						}
						else
						{
							TRACE("m_mapAutoClosed erase end!\r\n");
						}
						break;
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClosed.erase(head.hEvent);
			m_lock.unlock();
			if (initSocket() == false)
			{
				TRACE("init socket failed!\r\n");
			}
		}
		Sleep(1);
	}
	closeClient();

}
*/

void CClientSocket::threadFunc2()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message]) (msg.message, msg.wParam, msg.lParam);
		}
	}
}

bool CClientSocket::SendPacket(HWND hWnd, 
							   const CPacket& pack, 
							   bool isAutoClosed,
							   WPARAM wParam)
{
	if (m_hThread == INVALID_HANDLE_VALUE) {
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	}

	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);

	return PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam), (LPARAM)hWnd);
}

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	PACKET_DATA data = *(PACKET_DATA*)wParam;
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	if (initSocket() == true)
	{
		int ret = send(m_socket, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0)
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while(m_socket != INVALID_SOCKET)	
			{
				int length = recv(m_socket, pBuffer + index, BUFFER_SIZE - index, 0);
				if ( (length > 0) || (index > 0) )
				{
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, index);
					if (nLen > 0)
					{
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMode & CSM_AUTOCLOSE)
						{
							closeClient();
							return;
						}
					}
					index -= nLen;
					memmove(pBuffer, pBuffer + index, nLen);
				}
				else
				{
					closeClient();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
			}
		}
		else
		{
			TRACE("send msg size <= 0!\r\n");
			closeClient();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
		}
	}
	else
	{
		TRACE("init socket failed!\r\n");
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}

