#pragma once
#include "LThread.h"
#include <map>
#include <WinSock2.h>
#include <Ws2tcpip.h>
#include "LQueue.h"
#include <MSWSock.h>
    

enum LOperator {
    LNone,
    LAccept,
    LRecv,
    LSend,
    LError
};

class LServer;
class LClient;
typedef std::shared_ptr<LClient> PCLIENT;


class LOverlapped
{
public:
    OVERLAPPED m_overlapped;
    DWORD m_operator;           //操作
    std::vector<char> m_buffer; //缓冲区
    ThreadWorker m_worker;      //处理函数
    LServer* m_server;          //服务器对象
    LClient* m_client;
    WSABUF m_wsabuffer;
    virtual ~LOverlapped() {
        m_buffer.clear();
    }
};

template<LOperator>class AcceptOverlapped;
typedef AcceptOverlapped<LAccept> ACCEPTOVERLAPPED;

template<LOperator>class RecvOverlapped;
typedef RecvOverlapped<LRecv> RECVOVERLAPPED;

template<LOperator>class SendOverlapped;
typedef SendOverlapped<LSend> SENDOVERLAPPED;



class LClient : ThreadFuncBase
{
public:
    LClient();

    ~LClient()
    {
        closesocket(m_sock);
        m_recv.reset();
        m_send.reset();
        m_overlapped.reset();
        m_buffer.clear();
        m_vecSend.Clear();
    }

    void SetOverlapped(PCLIENT& ptr);

    operator SOCKET()
    {
        return m_sock;
    }

    operator PVOID()
    {
        return &m_buffer[0];
    }

    operator LPOVERLAPPED();

    operator LPDWORD();

    LPWSABUF RecvWSABuffer();

    LPWSABUF SendWSABuffer();

    DWORD& flags() { return m_flags; }

    sockaddr_in* GetLocalAddr() { return &m_laddr; }
    sockaddr_in* GetRemoteAddr() { return &m_raddr; }
    
    size_t GetBufferSize() const { return m_buffer.size(); }

    int Recv();

    int Send(void* buffer, size_t nSize);

    int SendData(std::vector<char>& data);

private:
    SOCKET m_sock;
    DWORD  m_received;
    DWORD  m_flags;
    
    std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;
    std::shared_ptr<RECVOVERLAPPED>   m_recv;
    std::shared_ptr<SENDOVERLAPPED>   m_send;

    std::vector<char> m_buffer;
    size_t m_used;
    sockaddr_in m_laddr;
    sockaddr_in m_raddr;
    bool m_isbusy;
    LSendQueue<std::vector<char>> m_vecSend;//发送数据队列
};


template<LOperator>
class AcceptOverlapped : public LOverlapped, ThreadFuncBase
{
public:
    AcceptOverlapped()
    {
        m_operator = LAccept;
        m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped::AcceptWorker);
        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
        m_server = NULL;
    }
    int AcceptWorker();
};




template<LOperator>
class RecvOverlapped : public LOverlapped, ThreadFuncBase
{
public:
    RecvOverlapped();

    int RecvWorker()
    {
        int ret = m_client->Recv();
        return ret;
    }
};

template<LOperator>
class SendOverlapped : public LOverlapped, ThreadFuncBase
{
public:
    SendOverlapped();
    int SendWorker()
    {
        //TODO:
        /*
        * Send可能不会立即完成
        */
        return -1;
    }
};


template<LOperator>
class ErrorOverlapped : public LOverlapped, ThreadFuncBase
{
public:
    ErrorOverlapped()
        : m_operator(LError), m_worker(this, &ErrorOverlapped::ErrorWorker)
    {

        memset(&m_overlapped, 0, sizeof(m_overlapped));
        m_buffer.resize(1024);
    }
    int ErrorWorker()
    {

    }
};
typedef ErrorOverlapped<LError> ERROROVERLAPPED;



class LServer :
    public ThreadFuncBase
{
public:
    LServer(const std::string& ip = "0.0.0.0", short port = 9527)
        : m_pool(10)
    {
        m_hIOCP = INVALID_HANDLE_VALUE;

        memset(&m_addr, 0, sizeof(sockaddr_in));
        m_addr.sin_family = AF_INET;
        inet_pton(AF_INET, ip.c_str(), &m_addr.sin_addr.s_addr);
    }
    ~LServer();

    bool StartService()
    {
        CreateSocket();

        //bind 
        if (bind(m_sock, (sockaddr*)&m_addr, sizeof(m_addr)) == -1)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        if (listen(m_sock, 3) == -1)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            return false;
        }
        m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
        if (m_hIOCP == NULL)
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
        CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
        m_pool.Invoke();
        m_pool.DispathWorker(ThreadWorker(this, (FUNCTYPE)&LServer::threadIocp));

        if (!NewAccept()) return false;

        return true;
    }

    bool NewAccept()
    {
        PCLIENT pClient(new LClient());
        pClient->SetOverlapped(pClient);
        m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
        if (!AcceptEx(m_sock,
            *pClient,
            *pClient,
            0,
            sizeof(sockaddr_in) + 16,
            sizeof(sockaddr_in) + 16,
            *pClient, *pClient))
        {
            closesocket(m_sock);
            m_sock = INVALID_SOCKET;
            m_hIOCP = INVALID_HANDLE_VALUE;
            return false;
        }
    }

private:
    void CreateSocket()
    {
        m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        int opt = 1;
        setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    }


    int threadIocp();


private:
    LThreadPool m_pool;
    HANDLE m_hIOCP;
    SOCKET m_sock;
    sockaddr_in m_addr;
    std::map<SOCKET, std::shared_ptr<LClient>> m_client;
    //LQueue<LClient> m_lstClient;
};


