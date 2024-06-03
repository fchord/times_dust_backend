#pragma once
#include <string>

//#define WIN

//bool isUsableDir(std::string sPath);
//bool isUsableDirW(std::wstring sPath);
//std::wstring getCurDirW();
std::wstring string2wstring(std::string str);
std::string wstring2string(std::wstring wstr);
int rm(std::wstring wsFile);