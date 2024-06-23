#pragma once
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
};

