#include "pch.h"
#include "LServer.h"
#include "Tool.h"

template<LOperator op>
int  AcceptOverlapped<op>::AcceptWorker()
{
    INT lLength = 0, rLength = 0;
    if (*(LPDWORD)*m_client > 0)
    {
        GetAcceptExSockaddrs
        (*m_client, 0,
            sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength,
            (sockaddr**)m_client->GetRemoteAddr(), &rLength
        );

        int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), *m_client, NULL);
        if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING))
        {
            //TODO: 报错
        }
        
            
       
        if (!m_server->NewAccept())
        {
            return -2;
        }
    }
    return -1;
}

LClient::LClient()
	: m_isbusy(false),
	m_flags(0),
	m_recv(new RECVOVERLAPPED()),
	m_send(new SENDOVERLAPPED()),
	m_overlapped(new ACCEPTOVERLAPPED()),
	m_vecSend(this, (SENDCALLBACK)&LClient::SendData)
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}

void LClient::SetOverlapped(PCLIENT& ptr)
{
    m_overlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();
}


LClient::operator LPOVERLAPPED()
{
    return &m_overlapped->m_overlapped;
}

LClient::operator LPDWORD() {
    return &m_received;
}

LPWSABUF LClient::RecvWSABuffer()
{
	return &m_recv->m_wsabuffer;
}

LPWSABUF LClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}

int LClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0) return -1;
	m_used += (size_t)ret;
	//TODO:解析数据
	return 0;
}

int LClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data))
	{
		return 0;
	}
	return -1;
}

int LClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0)
	{
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING))
		{
			Tool::ShowError();
			return -1;
		}
	}
	return 0;
}

LServer::~LServer()
{
	closesocket(m_sock);
	std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
	for (; it != m_client.end(); it++)
	{
		it->second.reset();
	}
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
}

int LServer::threadIocp()
{
	DWORD transferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
	{
		if ((transferred > 0) && (CompletionKey != 0))
		{
			LOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, LOverlapped, m_overlapped);
			switch (pOverlapped->m_operator)
			{
			case LAccept:
			{
				ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
				m_pool.DispathWorker(pAccept->m_worker);
			}
			break;

			case LRecv:
			{
				RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
				m_pool.DispathWorker(pOver->m_worker);
			}
			break;

			case LSend:
			{
				SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
				m_pool.DispathWorker(pSend->m_worker);
			}
			break;

			case LError:
			{
				ERROROVERLAPPED* pError = (ERROROVERLAPPED*)pOverlapped;
				m_pool.DispathWorker(pError->m_worker);
			}
			break;
			}
		}
	}
	return 0;

}


template<LOperator op>
inline SendOverlapped<op>::SendOverlapped()
{
	m_operator = LSend;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}



template<LOperator op>
inline RecvOverlapped<op>::RecvOverlapped()
{

	m_operator = LRecv;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);

}
