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
		if (nLength + i > nSize)	//��δ��ȫ���յ��� �ͷ��أ� ����ʧ��
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
		strOut.resize(nLength + 6);	//Ϊʲô����Ҫ��������һ��������

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
	WORD sHead;				//�̶�FE FF
	DWORD nLength;			//������
	WORD sCmd;				//��������
	std::string strData;	//������
	WORD sSum;				//��У��
	std::string strOut;		//���ݰ���ȫ������
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
	WORD nAction;	//������ƶ���˫��
	WORD nButton;	//������Ҽ����м��
	POINT ptXY;		//����
}MOUSEEV, * PMOUSEEV;

typedef struct file_info {
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;      //�Ƿ���Ч
	BOOL HasNext;
	BOOL IsDirectory;    //�Ƿ�ΪĿ¼0 �� 1��
	char szFileName[256];//�ļ���
}FILEINFO, * PFILEINFO;