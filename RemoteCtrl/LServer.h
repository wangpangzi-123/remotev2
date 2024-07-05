#pragma once
#include "LThread.h"
#include <map>
#include <WinSock2.h>
#include <Ws2tcpip.h>

class LClient
{

};

enum LOperator{
    LNone,
    LAccept,
    LRecv,
    LSend,
    LError
};

class LOverlapped
{
public:
    OVERLAPPED m_overlapped;    
    DWORD m_operator;           //操作
    std::vector<char> m_buffer; //缓冲区
    ThreadWorker m_worker;      //处理函数
};

template<LOperator>
class AcceptOverlapped : public LOverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped() 
        : m_operator(LAccept), m_worker()
    {
    }
    
};

class LServer :
    public ThreadFuncBase
{
public:
    LServer(const std::string& ip = "0.0.0.0", short port = 9527)
        : m_pool(10)
    {
        m_hIOCP = INVALID_HANDLE_VALUE;
        m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

        sockaddr_in addr;
        memset(&addr, 0, sizeof(sockaddr_in));
        addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &addr.sin_addr.s_addr);

        //bind 
        if (bind(m_sock, (sockaddr*)&addr, sizeof(addr)) == -1)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return;
        }
        if (listen(m_sock, 3) == -1)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return;
        }
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        if (m_hIOCP == NULL)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
        m_pool.DispathWorker(ThreadWorker(this, (FUNCTYPE)&LServer::threadIocp));
    }

    ~LServer() {}

private:
    int threadIocp()
    {
        DWORD transferred = 0;
        ULONG_PTR CompletionKey = 0;
        OVERLAPPED* lpOVerlapped = NULL;
        if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &CompletionKey, &lpOVerlapped, INFINITE))
        {
            if ((transferred > 0) && (CompletionKey != 0))
            {

            }
            CONTAINING_RECORD(lpOVerlapped, LOverlapped, m_overlapped)
        }
        return 0;
    }

private:
    LThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    std::map<SOCKET, std::shared_ptr<LClient>> m_client;
};

