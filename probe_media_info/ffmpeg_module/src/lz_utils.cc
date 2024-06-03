#include "lz_utils.h"
#include <iostream>
#include <fstream>
#include <string>


#ifdef WIN
//#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define ST_MODE_NOT_EXISTENCE 0
#define ST_MODE_DIR 1
#define ST_MODE_FILE 2
#define ST_MODE_OTHER 3
#else
#include <locale>
#include <codecvt>
#endif

using namespace std;

int s_iThreshold = 10 * 1024 * 1024;
std::wstring s_sPath;

#if 0
bool isUsableDir(std::string sPath)
{

	int type, n;
	struct stat s;
	char *path = new char[sPath.length() + 2];
	memset(path, 0, sPath.length() + 2);
	strncpy_s(path, sPath.length() + 1, sPath.c_str(), sPath.length());
#ifdef WIN
	for (int i = 0; i < strlen(path); i++)
		if (path[i] == '\\')
			path[i] = '/';
	if ('/' != path[strlen(path) - 1]) {
		path[strlen(path)] = '/';
}
#endif

	path[strlen(path)] = '\0';

	cout << "isUsableDir. path: " << path << endl;

#ifdef WIN
	// Is it a dir ?
	if ((n = stat(path, &s)) == 0) {
		if (s.st_mode & S_IFDIR) {
			type = ST_MODE_DIR;
		}
		else if (s.st_mode & S_IFREG) {
			type = ST_MODE_FILE;
		}
		else {
			type = ST_MODE_OTHER;
		}
	}
	else {
		cout << "stat: " << errno << endl;
		type = ST_MODE_NOT_EXISTENCE;
	}
	if (type != ST_MODE_DIR) {
		cout << "isUsableDir. It 's not dir." << endl;
		delete[] path;
		path = NULL;
		return false;
	}
#endif

	std::string sTest = "test";
	std::string sPathStr(path);
	ofstream of_test((sPathStr + sTest).c_str(), std::ios::app, ios_base::_Openprot);

	delete[] path;
	path = NULL;

	if (!of_test) {
		cout << "isUsableDir. Build test file failed." << endl;
		return false;
	}
	else {
		cout << "isUsableDir. It 's usable dir." << endl;
		of_test.close();
#ifdef WIN
		n = remove((sPathStr + sTest).c_str());
#endif
		if (n < 0)
			cout << "remove " << (sPathStr + sTest).c_str() << " failed, n: " << n
			<< ", errno: " << errno << endl;
	}
	return true;
}

bool isUsableDirW(std::wstring sPath)
{
	int type, n, len;
	struct _stat64i32 s;
	len = sPath.length() + 2;
	wchar_t *path = new wchar_t[len];
	memset(path, 0, len * sizeof(wchar_t));
	
#ifdef WIN
	lstrcpynW(path, sPath.c_str(), sPath.length() + 1);

	for (int i = 0; i < wcslen(path); i++)
		if (path[i] == L'\\')
			path[i] = L'/';
	if (L'/' != path[wcslen(path) - 1]) {
		path[wcslen(path)] = L'/';
	}
#endif

	path[wcslen(path)] = L'\0';

	wcout << L"isUsableDirW. path: " << path << endl;

#ifdef WIN
	// Is it a dir ?
	if ((n = _wstat(path, &s)) == 0) {
		if (s.st_mode & S_IFDIR) {
			type = ST_MODE_DIR;
		}
		else if (s.st_mode & S_IFREG) {
			type = ST_MODE_FILE;
		}
		else {
			type = ST_MODE_OTHER;
		}
	}
	else {
		cout << "stat: " << errno << endl;
		type = ST_MODE_NOT_EXISTENCE;
	}
	if (type != ST_MODE_DIR) {
		cout << "isUsableDirW. It 's not dir." << endl;
		delete[] path;
		path = NULL;
		return false;
	}
#endif

	std::wstring sTest = L"test";
	std::wstring sPathStr(path);
	wofstream of_test((sPathStr + sTest).c_str(), std::ios::app, ios_base::_Openprot);

	delete[] path;
	path = NULL;

	if (!of_test) {
		cout << "isUsableDirW. Build test file failed." << endl;
		return false;
	}
	else {
		cout << "isUsableDirW. It 's usable dir." << endl;
		of_test.close();
#ifdef WIN
		n = _wremove((sPathStr + sTest).c_str());
#endif
		if (n < 0)
			wcout << L"remove " << (sPathStr + sTest).c_str() << L" failed, n: " << n
			<< L", errno: " << errno << endl;
	}
	return true;

}

wstring getCurDirW()
{
#ifdef WIN
	wchar_t *buf = NULL;
	DWORD d = 256, dRet;
	do
	{
		buf = new wchar_t[d];
		dRet = GetModuleFileName(NULL, buf, d);
		// failed
		if (0 == dRet) {
			break;
		}
		// buffer is too small
		else if (d == dRet && ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
			delete[] buf;
			d = d * 2;
		}
		else {
			break;
		}
	} while (1);

	if (dRet > 0) {
		std::wstring sCurDir(buf);
		int i = sCurDir.length() - 1;
		while (sCurDir[i] != L'\\')
			i--;
		sCurDir = sCurDir.substr(0, i + 1);
		delete[] buf;
		return sCurDir;
	}
	else {
		delete[] buf;
		return std::wstring(L"C:\\");
	}
#endif
}
#endif

std::wstring string2wstring(std::string str)
{
#ifdef WIN
	std::wstring result;
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);  //获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	wchar_t* buffer = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), buffer, len);  //多字节编码转换成宽字节编码  
	buffer[len] = L'\0';
	result.append(buffer);
	delete[] buffer;
	return result;
#else
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
#endif
}

std::string wstring2string(std::wstring wstr)
{
#ifdef WIN
	string result;
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);  //获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	char* buffer = new char[len + 1];
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);  //宽字节编码转换成多字节编码  
	buffer[len] = '\0';
	result.append(buffer);
	delete[] buffer;
	return result;
#else
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.to_bytes(wstr);	
#endif
}

int rm(std::wstring wsFile)
{
#ifdef WIN
	return _wremove(wsFile.c_str());
#endif
}