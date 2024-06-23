#pragma once

#include <Windows.h>
#include <string>
#include <atlimage.h>


class Tool
{
public:
	static void Dump(BYTE* pData, size_t nSize)	//应该加入 static 
	{
		std::string strOut = "Packet: ";
		for (int i = 0; i < nSize; i++)
		{
			char hexData[8];
			snprintf(hexData, sizeof(hexData), " %02X ", pData[i] & 0xFF);
			strOut += hexData;
		}
		OutputDebugStringA(strOut.c_str());
	}

	static int Bytes2Image(CImage& image,
							const std::string& strBuffer)
	{
		BYTE* pData = (BYTE*)strBuffer.c_str();
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);
		if (hMem == NULL)
		{
			TRACE("内存不足！");
			Sleep(1);
			return -1;
		}
		IStream* pStream = NULL;
		HRESULT hRet = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (hRet == S_OK)
		{
			ULONG length = 0;
			pStream->Write(pData,
				strBuffer.size(),
				&length);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			if ((HBITMAP)image != NULL)
				image.Destroy();
			image.Load(pStream);
			return 0;
		}
		return -2;
	}
};

