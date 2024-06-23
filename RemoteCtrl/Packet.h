#pragma once
#include "pch.h"
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
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;
		//std::string outputData = "server : ";
		//for (int ri = 0; ri < nSize; ri++)
		//{
		//	char rawData[8] = "";
		//	snprintf(rawData, sizeof(rawData), "%02X ", pData[ri] & 0xff);
		//	outputData += rawData;
		//}
		//OutputDebugStringA(outputData.c_str());

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
		//TRACE("Server recv sCmd = %d\r\n", sCmd);
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
	const char* Data()
	{
		strOut.resize(nLength + 6);	//为什么这里要重新设置一个缓冲区

		BYTE* pData = (BYTE*)strOut.c_str();

		*(WORD*)pData = sHead;		pData += 2;
		*(DWORD*)pData = nLength;   pData += 4;
		*(WORD*)pData = sCmd;		pData += 2;
		/*memcpy((void*)pData, &strData, strData.size()); pData += strData.size();*/
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();

		*(WORD*)pData = sSum;		pData += 2;
		return strOut.c_str();
	}

public:
	WORD sHead;				//固定FE FF
	DWORD nLength;			//包长度
	WORD sCmd;				//控制命令
	std::string strData;	//包数据
	WORD sSum;				//和校验
	std::string strOut;		//数据包的全部数据
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