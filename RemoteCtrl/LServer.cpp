#include "pch.h"
#include "LServer.h"

template<LOperator op>
int  AcceptOverlapped<op>::AcceptWorker()
{
    INT lLength = 0, rLength = 0;
    if (*(LPDWORD)*m_client.get() > 0)
    {
        GetAcceptExSockaddrs
        (*m_client, 0,
            sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
            (sockaddr**)m_client->GetLocalAddr(), &lLength,
            (sockaddr**)m_client->GetRemoteAddr(), &rLength
        );

        if (WSARecv((SOCKET)*m_client, *m_client, 1, *m_client, &m_client->flags(), *m_client, NULL))
        {
            
        }

        if (!m_server->NewAccept())
        {
            return -2;
        }
    }
    return -1;
}

LClient::LClient()
    : m_isbusy(false), m_overlapped(new ACCEPTOVERLAPPED()), m_flags(0)
{
    m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_buffer.resize(1024);
    memset(&m_laddr, 0, sizeof(m_laddr));
    memset(&m_raddr, 0, sizeof(m_raddr));
}

void LClient::SetOverlapped(PCLIENT& ptr)
{
    m_overlapped->m_client = ptr;
}


LClient::operator LPOVERLAPPED()
{
    return &m_overlapped->m_overlapped;
}

LClient::operator LPDWORD() {
    return &m_received;
}

