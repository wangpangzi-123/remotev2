#pragma once

#include "pch.h"
#include "framework.h"
#include <iostream>
#include <Ws2tcpip.h>
#include <vector>
#include <iomanip>
#include <unordered_map>
#include <map>
#include <mutex>
//声明 在 .cpp 


#define WM_SEND_PACK		(WM_USER + 1)	//发送包数据
#define WM_SEND_PACK_ACK	(WM_USER + 2)	


#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket() : sHead(0), nLength(0), sCmd(0), sSum(0) {}

	CPacket(WORD nCmd, const BYTE* pData, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (int i = 0; i < nSize; i++)
		{
			sSum += BYTE(strData[i]) & 0xff;
		}
	}

	CPacket(const CPacket& pack)
	{
		sHead = pack.sHead;
		nLength = pack.nLength;
		sCmd = pack.sCmd;
		strData = pack.strData;
		sSum = pack.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize)
	{
		size_t i = 0;
		for (; i < nSize; i++)
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)	//包未完全接收到， 就返回， 解析失败
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)
		{
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0XFF;
		}
		if (sum == sSum) {
			nSize = i; //head 2 length 4 data ...
			return;
		}
		nSize = 0;
	}
	CPacket& operator=(const CPacket& pack)
	{
		if (this != &pack)
		{
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	~CPacket() {}

	int Size()
	{
		return nLength + 6;
	}
	void Data(std::string& strOut) const
	{
		strOut.resize(nLength + 6);	//为什么这里要重新设置一个缓冲区

		BYTE* pData = (BYTE*)strOut.c_str();

		*(WORD*)pData = sHead;		pData += 2;
		*(DWORD*)pData = nLength;   pData += 4;
		*(WORD*)pData = sCmd;		pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;		pData += 2;
		//return strOut.c_str();
	}

public:
	WORD sHead;				//固定FE FF
	DWORD nLength;			//包长度
	WORD sCmd;				//控制命令
	std::string strData;	//包数据
	WORD sSum;				//和校验
	//std::string strOut;		//数据包的全部数据
};
#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;	//点击、移动、双击
	WORD nButton;	//左键、右键、中间键
	POINT ptXY;		//坐标
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;      //是否有效
	BOOL HasNext;
	BOOL IsDirectory;    //是否为目录0 否 1是
	char szFileName[256];//文件名
}FILEINFO, * PFILEINFO;


enum {
	CSM_AUTOCLOSE = 1,	// CSM = Client Socket Mode 自动关闭
	S
};


typedef struct PacketData {
	std::string strData;
	UINT nMode;
	PacketData(const char* pData, size_t nLen, UINT mode)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
	}

	PacketData(const PacketData& data)
	{
		strData = data.strData;
		nMode = data.nMode;
	}

	PacketData& operator=(const PacketData& data)
	{
		if (this != &data)
		{
			strData = data.strData;
			nMode = data.nMode;
		}
		return *this;
	}

}PACKET_DATA;


std::string GetErrInfo(int wsaErrCode);

class CClientSocket
{
public:
	
	static CClientSocket& getInstance()	//通过 static 静态函数去调用类中的对象, 静态函数没有 this 指针
	{
		static CClientSocket m_instance;
		return m_instance;
	}

	/*bool initSocket(const std::string& strIPAddress, int nPort)*/
	bool initSocket()
	{
		if (m_socket != INVALID_SOCKET) closesocket(m_socket);

		m_socket = socket(AF_INET, SOCK_STREAM, 0);

		//TRACE("client m_socket:%d\r\n", m_socket);
		if (m_socket == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));

		serv_adr.sin_family = AF_INET;
		//inet_pton(AF_INET, "127.0.0.1", &serv_adr.sin_addr.s_addr);
		//TRACE("ADDR %08X %08X \r\n", serv_adr.sin_addr.s_addr, htonl(nIP));
		serv_adr.sin_addr.s_addr = htonl(m_nIP);
		serv_adr.sin_port = htons(m_nPort);

		if (serv_adr.sin_addr.s_addr == INADDR_NONE)
		{
			AfxMessageBox(" 指定的ip 地址不存在 ！ ");
			return false;
		}

		int ret = connect(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr));
		if (ret == -1)
		{
			AfxMessageBox(" 连接失败 ！ ");
			TRACE("%s(%d)  client connect error! \r\n %d %s\r\n", 
				__FUNCTION__, __LINE__, 
				WSAGetLastError(), 
				GetErrInfo(WSAGetLastError()).c_str());
			return false;
		}
		return true;
	}


#define BUFFER_SIZE 3276800
	int dealCommand()
	{
		if (m_socket == -1) return -1;
		//char* buffer = new char[4096];
		char* buffer = m_buffer.data();
		static size_t index = 0;

		while (true)
		{
			size_t len = recv(m_socket, buffer + index, BUFFER_SIZE - index, 0);

			//TRACE("client recv packet len = %ld\r\n", len);
			if ((len <= 0) && (index <= 0))
			{
				return -1;
			}
			//TODO:：处理命令
			index += len;
			len = index;
			m_packet = CPacket((const BYTE*)buffer, (size_t&)len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	CPacket& getPacket(void)
	{
		return m_packet;
	}

	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true);


/*
	bool SendPacket(const CPacket& pack, 
		std::list<CPacket>& lstPacks,
		bool isAutoClosed = true)
	{
		if (m_socket == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
		{
			//bool initResult = initSocket();
			//if (initResult == false)
			//{
			//	TRACE("init socket false \r\n");
			//	return false;
			//}
			m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
		}


		m_lock.lock();
		m_lstSend.push_back(pack);
		
		auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>
			(pack.hEvent, lstPacks));
		m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
		m_lock.unlock();


		WaitForSingleObject(pack.hEvent, INFINITE);

		std::map<HANDLE, std::list<CPacket>&>::iterator it;
		it = m_mapAck.find(pack.hEvent);
	
		if (it != m_mapAck.end())
		{

			//std::list<CPacket>::iterator i;
			//for (i = it->second.begin(); i != it->second.end(); i++)
			//{
			//	lstPacks.push_back(*i);
			//}
			TRACE("SendPacket sc!\r\n");

			m_lock.lock();
			m_mapAck.erase(pack.hEvent);
			m_lock.unlock();
			
			return true;
		}
		TRACE("SendPacket false!\r\n");
		//m_mapAck.erase(pack.hEvent);
		return false;
	}

*/

	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}

	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	int closeClient()
	{
		int ret = closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		return ret;
	}

	void UpdateAddress(int nIP, int nPort)
	{
		if ((m_nIP != nIP) || (m_nPort != nPort))
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}


private:
	bool Send(const char* pData, int nSize)
	{
		if (m_socket == -1) return false;
		return send(m_socket, pData, nSize, 0) > 0;
	}

	bool Send(const CPacket& packet)
	{
		if (m_socket == -1) return false;
		TRACE("m_socket = %d\r\n", m_socket);
		std::string strOut;
		packet.Data(strOut);
		return send(m_socket, strOut.c_str(), strOut.size(), 0) > 0;
	}


private:
	UINT m_nThreadID;

	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;

	void SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	


	
	HANDLE m_hThread;
	bool m_bAutoClose;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;
	std::map<HANDLE, std::list<CPacket>&>  m_mapAck;
	std::map<HANDLE, bool> m_mapAutoClosed;

	int m_nIP;		//地址
	int m_nPort;	//端口

	std::vector<char> m_buffer;
	SOCKET m_socket;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss)	//赋值构造函数
	{

	}

	CClientSocket(const CClientSocket& ss);	//复制构造函数

	CClientSocket()
		: m_nIP(INADDR_ANY), 
		  m_nPort(0),
		  m_socket(INVALID_SOCKET),
		  m_bAutoClose(true),
		  m_hThread(INVALID_HANDLE_VALUE)
	{
		/*m_socket = INVALID_SOCKET;*/
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化 套接字环境， 请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);	//硬件故障
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);

		struct
		{
			UINT message;
			MSGFUNC func;
		}
		funcs[] =
		{
			{WM_SEND_PACK, &CClientSocket::SendPack},
			{0, NULL}
		};
		for (int i = 0; funcs[i].message != 0; i++)
		{
			if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
			{
				TRACE("m_mapFunc insert  error! -> func[%d].message = %d, func[%d].func = %08X\r\n",
					i, funcs[i].message, i, funcs[i].func);
			}
		}
	}

	virtual ~CClientSocket()
	{
		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
		WSACleanup();
	}

	static unsigned __stdcall threadEntry(void* arg);

	void threadFunc();

	void threadFunc2();

	bool InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
};
