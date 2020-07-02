#include "stdafx.h"
#include "ImgCvt.h"

#include <io.h>

bool FileExists(const char *filePathPtr)
{
	char filePath[_MAX_PATH];

	// Strip quotation marks (if any)
	if (filePathPtr[0] == '"')
	{
		strcpy(filePath, filePathPtr + 1);
	}
	else
	{
		strcpy(filePath, filePathPtr);
	}

	// Strip quotation marks (if any)
	if (filePath[strlen(filePath) - 1] == '"')
		filePath[strlen(filePath) - 1] = 0;

	return (_access(filePath, 0) != -1);
}

std::string toString(float v) {
	char buf[256];

	sprintf(buf, "%d", (int)v);
	return std::string(buf);
}

bool isLittleEndian()
{
	uint16_t number = 0x1;
	char *numPtr = (char*)&number;
	return (numPtr[0] == 1);
}

char readChar(FILE *pFile) {
	char c;
	int bytesRead = fread(&c, 1, 1, pFile);
	if (bytesRead != 1)
		return -1;
	return c;
}

std::string readString(FILE *pFile, uint8_t length) {
	char *c = (char *)calloc(1, length + 1);

	int bytesRead = fread(c, length, 1, pFile);
	if (bytesRead != 1) {
		free(c);
		return std::string("");
	}
	std::string ret(c);
	free(c);
	return ret;
}

uint32_t readUint32(FILE *pFILE) {
	uint32_t ret;

	int bytesRead = fread(&ret, sizeof(uint32_t), 1, pFILE);
	if (bytesRead != 1)
		return -1;
	return ret;
}

uint64_t readUint64(FILE *pFILE) {
	uint64_t ret;

	int bytesRead = fread(&ret, sizeof(uint64_t), 1, pFILE);
	if (bytesRead != 1)
		return -1;
	return ret;
}

uint16_t readUint16(FILE *pFile) {
	uint16_t ret;

	int bytesRead = fread(&ret, sizeof(uint16_t), 1, pFile);
	if (bytesRead != 1)
		return -1;
	return ret;
}


float readFloat(FILE *pFile)
{
	float f;
	char *c = (char *)(&f);
	int bytesRead = 0;

	if (isLittleEndian()) {
		bytesRead = fread(&f, sizeof(float), 1, pFile);
	}
	else {
		c[3] = readChar(pFile);
		if (c[3] != -1) return -1;
		c[2] = readChar(pFile);
		if (c[2] != -1) return -1;
		c[1] = readChar(pFile);
		if (c[1] != -1) return -1;
		c[0] = readChar(pFile);
		if (c[0] != -1) return -1;
		bytesRead = 1;
	}
	if (bytesRead != 1)
		return -1;
	return f;
}

double readDouble(FILE *pFile)
{
	double d;
	char *c = (char *)(&d);
	int bytesRead = 0;

	if (isLittleEndian()) {
		bytesRead = fread(&d, sizeof(double), 1, pFile);
	}
	else {
		c[7] = readChar(pFile);
		if (c[7] != -1) return -1;
		c[6] = readChar(pFile);
		if (c[6] != -1) return -1;
		c[5] = readChar(pFile);
		if (c[5] != -1) return -1;
		c[4] = readChar(pFile);
		if (c[4] != -1) return -1;
		c[3] = readChar(pFile);
		if (c[3] != -1) return -1;
		c[2] = readChar(pFile);
		if (c[2] != -1) return -1;
		c[1] = readChar(pFile);
		if (c[1] != -1) return -1;
		c[0] = readChar(pFile);
		if (c[0] != -1) return -1;
		bytesRead = 1;
	}
	if (bytesRead != 1)
		return -1;
	return d;
}

void log(FBXENUM msg) {
	switch (msg)
	{
	case SUCCESS_IMPORT:
		MessageBox(NULL, "Successfully Imported(FBX)!", "3D Model Master", MB_OK);
		break;
	case INVALID_FBX_FILE:
		MessageBox(NULL, "Invalid or Damaged FBX Found!", "3D Model Master", MB_OK);
		break;
	case UNSURPPORTED_FBX_VERSION:
		MessageBox(NULL, "Unsupported FBX Version!", "3D Model Master", MB_OK);
		break;
	case UNSURPPORTED_PROPERTY_TYPE:
		MessageBox(NULL, "Unsupported Property Found(FBX)!", "3D Model Master", MB_OK);
		break;
	case COMPRESSED_PROPERTY_FOUND:
		MessageBox(NULL, "Compressed Property Found(FBX)!", "3D Model Master", MB_OK);
		break;
	default:
		break;
	}
}