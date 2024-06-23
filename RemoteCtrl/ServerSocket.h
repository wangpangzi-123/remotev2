#pragma once

#include "pch.h"
#include "framework.h"
#include <iostream>
#include <memory>
#include <vector>
	//声明 在 .cpp 
#include <list>
#include "Packet.h"


typedef void(*SOCKET_CALLBACK)(void* arg, int status, 
							   std::list<CPacket>& lstPacket,
							   CPacket& inPacket);
//using SOCKET_CALLBACK = int(*)(void* arg);

class CServerSocket
{
public:
	void Dump(const char* pData, size_t nSize)
	{
		std::string strOut;
		for (size_t i = 0; i < nSize; i++)
		{
			char buf[8] = "";
			snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xff);
			strOut += buf;
		}
		strOut += "\r\n";
		OutputDebugStringA(strOut.c_str());
	}

public:
	static CServerSocket* getInstance()	//通过 static 静态函数去调用类中的对象, 静态函数没有 this 指针
	{
		if (m_instance == NULL)
		{
			m_instance = new CServerSocket();
		}
		return m_instance;
	}

	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527)
	{
		bool ret = initSocket(port);
		if (ret == false) return -1;
		
		m_callback = callback;
		m_arg = arg;
		
		std::list<CPacket> lstPacket;

		int count = 0;
		while (true)
		{
			if (acceptClient() == false)
			{
				if (count >= 3)
				{
					return -2;
				}
				count++;
			}
			else
			{
				int ret = dealCommand();
				if (ret > 0)
				{
					m_callback(m_arg, ret, lstPacket, m_packet);
					while (lstPacket.size() > 0)
					{
						Send(lstPacket.front());
						lstPacket.pop_front();
					}
				}
			}
			closeClient();
		}
	}

protected:
	bool initSocket(short port)
	{
		if (m_socket == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));

		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;  // TODO: 服务器
		serv_adr.sin_port = htons(port);

		if (bind(m_socket, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)  // TODO: 返回值处理
		{
			return false;
		}
		//绑定
		if (listen(m_socket, 1) == -1)
		{
			return false;
		}

		return true;
	}

	bool acceptClient()
	{
		sockaddr_in client_adr;
		memset(&client_adr, 0, sizeof(client_adr));

		int client_adr_len = sizeof(client_adr);
		m_client = accept(m_socket, (sockaddr*)&client_adr, &client_adr_len); // TODO: 返回值处理
		//TRACE("m_client = %d\r\n", m_client);
		if (m_client == -1) return false;
		return true;
	}

#define BUFFER_SIZE 4096
	int dealCommand()
	{
		if (m_client == -1) return -1;
		/*char* buffer = new char[4096];*/
		/*std::unique_ptr<char[]> buffer = std::make_unique<char[]>(4096);*/
		char* buffer = m_buffer.data();
		
		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			if (len <= 0 && index == 0)
			{
				//delete[] buffer;
				return -1;
			}
			else
			{
				index += len;
				len = index;
				m_packet = CPacket((const BYTE*)buffer, (size_t&)len);
				//TRACE("server has recv msg len:%d, sCmd:%d\r\n", len, m_packet.sCmd);
				if (len > 0)
				{
					memmove(buffer, buffer + len, BUFFER_SIZE - index);
					index -= len;
					
	/*				delete[] buffer;*/
					return m_packet.sCmd;
				}
			}
		}
		//delete[] buffer;
		return -1;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1) return false;
		//Dump(pData, nSize);
		return send(m_client, pData, nSize, 0) > 0;
	}

	bool Send(CPacket& packet)
	{
		if (m_client == -1) return false;
		//Dump(packet.Data(), packet.Size());
		return send(m_client, packet.Data(), packet.Size(), 0) > 0;
	}
	
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4) || (m_packet.sCmd == 9))
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
		if (m_client != INVALID_SOCKET)
		{
			int ret = closesocket(m_client);
			m_client = INVALID_SOCKET;
			return ret;
		}
		return 0;
	}



private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;

	std::vector<char> m_buffer;
	SOCKET m_socket;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss)	//赋值构造函数
	{

	}	
	CServerSocket(const CServerSocket& ss)	//复制构造函数
	{
		m_socket = ss.m_socket;
		m_client = ss.m_client;
	}					
	CServerSocket() 
	{
		/*m_socket = INVALID_SOCKET;*/
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化 套接字环境， 请检查网络设置"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);	//硬件故障
		}
		m_socket = socket(AF_INET, SOCK_STREAM, 0);
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);
	}
	virtual ~CServerSocket() 
	{
		std::cout << "xigou" << std::endl;
		closesocket(m_socket);
		WSACleanup();
	}

	bool InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
	static void releaseInstance(void)
	{
		if (m_instance != NULL)
		{
			CServerSocket* tmp = m_instance;
			m_instance = NULL;
			delete tmp;
		}
	}
	static CServerSocket* m_instance;
	
	class CHelper {
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}
		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};
extern CServerSocket server;