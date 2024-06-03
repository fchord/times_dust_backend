#pragma once
//#include <windows.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>
#include <time.h>
#include <thread>
#include <mutex>
#include <list>
#include <vector>
#include <array>
#include <bitset>
#include <functional>

#define FFMPEG_LOG
#define LOG_TIMESTAMP
#define LOG_FSTREAM
#define LOG_COUT
//#define SOURCE_WITH_PATH

constexpr int arrLogSize = 4096;

void ffmpeglog(void* avcl, int level, const char* fmt, va_list vl);

enum LzLogLevel {
	LzLogLevelNull,
	LzLogLevelError,
	LzLogLevelFail,
	LzLogLevelInfo,
	LzLogLevelDebug
};

class Log;
class LogList;

class Log {
public:
	enum NumbericBase{
		Oct,
		Dec,
		Hex
	};
	Log(Log& log);
	Log(const char *src, const int line, unsigned int tid, LogList* loglist, LzLogLevel lvl, clock_t clk);
	~Log();

	std::string strSrc;
	int lineNumber;
	unsigned int threadId;
	LzLogLevel level;
	clock_t clock;
	std::string strLog;

	Log& operator<< (const bool i) {
		char c[12] = { '\0' };
		if(i)
			snprintf(c, 12, "true");
		else
			snprintf(c, 12, "false");
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const short i) {
		char c[12] = { '\0' };
		snprintf(c, 12, "%d", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const unsigned short i) {
		char c[12] = { '\0' };
		snprintf(c, 12, "%u", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const int i) {
		char c[12] = {'\0'};
		if(Dec == m_NumbericBase)
			snprintf(c, 12, "%d", i);
		else if (Hex == m_NumbericBase)
			snprintf(c, 12, "0x%X", i);
		else if (Oct == m_NumbericBase)
			snprintf(c, 12, "0%o", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const unsigned int i) {
		char c[12] = { '\0' };
		if (Dec == m_NumbericBase)
			snprintf(c, 12, "%u", i);
		else if (Hex == m_NumbericBase)
			snprintf(c, 12, "0x%X", i);
		else if (Oct == m_NumbericBase)
			snprintf(c, 12, "0%o", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const long i) {
		char c[12] = { '\0' };
		if (Dec == m_NumbericBase)
			snprintf(c, 12, "%d", i);
		else if (Hex == m_NumbericBase)
			snprintf(c, 12, "0x%X", i);
		else if (Oct == m_NumbericBase)
			snprintf(c, 12, "0%o", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const unsigned long i) {
		char c[12] = { '\0' };
		if (Dec == m_NumbericBase)
			snprintf(c, 12, "%u", i);
		else if (Hex == m_NumbericBase)
			snprintf(c, 12, "0x%X", i);
		else if (Oct == m_NumbericBase)
			snprintf(c, 12, "0%o", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const long long i) {
		char c[24] = { '\0' };
		if (Dec == m_NumbericBase)
			snprintf(c, 24, "%d", i);
		else if (Hex == m_NumbericBase)
			snprintf(c, 24, "0x%X", i);
		else if (Oct == m_NumbericBase)
			snprintf(c, 24, "0%o", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const unsigned long long i) {
		char c[24] = { '\0' };
		if (Dec == m_NumbericBase)
			snprintf(c, 24, "%u", i);
		else if (Hex == m_NumbericBase)
			snprintf(c, 24, "0x%X", i);
		else if (Oct == m_NumbericBase)
			snprintf(c, 24, "0%o", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const float i) {
		char c[12] = { '\0' };
		snprintf(c, 12, "%.6f", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const double i) {
		char c[24] = { '\0' };
		snprintf(c, 24, "%.12f", i);
		strLog.append(c);
		return *this;
	}
	Log& operator<< (const char *s) {
		strLog.append(s);
		return *this;
	}
	Log& operator<< (const std::string& s) {
		strLog.append(s);
		return *this;
	}
	Log& operator<< (const Log::NumbericBase& numberic) {
		m_NumbericBase = numberic;
		return *this;
	}
private:
	LogList* pList = NULL;
	bool m_toPush = true;

	NumbericBase m_NumbericBase = Dec;
};

class LogList {
public:
	LogList() { return; }
	~LogList() { return; }
	bool push(Log* log)
	{
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			m_ltLog.push_back(log);
			m_iTotalLen += log->strLog.length();
			++m_iSize;
		}
		return true;
	}

	Log* pop() {
		Log* log;
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			log = m_ltLog.front();
			m_ltLog.pop_front();
			m_iTotalLen -= log->strLog.length();
			--m_iSize;
		}
		return log;
	}
	int popLogs(std::array<Log*, arrLogSize>& arrLogs)
	{
		int i = 0;
		{
			std::lock_guard<std::mutex> lock(m_mtx);
			for (i = 0; i < arrLogSize && m_ltLog.size() > 0; i++)
			{
				arrLogs[i] = m_ltLog.front();
				m_ltLog.pop_front();
				m_iTotalLen -= arrLogs[i]->strLog.length();
				--m_iSize;
			}
		}
		return i;
	}
	int length() {
		return m_iTotalLen;
	}
	int size() {
		return m_iSize;
	}
private:
	int m_iTotalLen = 0;
	int m_iSize = 0;
	std::mutex m_mtx;
	std::list<Log*> m_ltLog;
};


class LzLog {
public:
	static void Init(std::wstring path);
	static void Uninit();
	static LogList* s_logList;
private:
	LzLog() {
		m_thFlush = new std::thread(std::mem_fn(&LzLog::flushLog), this);
		m_thPop = new std::thread(std::mem_fn(&LzLog::popLogList), this);
		return;
	}
	~LzLog() {
		bQuit = true;
		if (m_thFlush && m_thFlush->joinable())
		{
			m_thFlush->join();
		}
		return;
	}

	static LzLog* sm_pLzLog;
	static std::wstring sm_wsPath;
	static std::wofstream fsLog;

	std::mutex m_mtx;

	bool bQuit = false;
	bool bPopQuit = false;
	std::thread* m_thPop;

	std::mutex m_mtxBufs;
	std::list<char *> m_ltBufs;
	std::thread* m_thFlush;

	void popLogList();
	void flushLog();
	
};


#define LzLogNull  Log(__FILE__, __LINE__, 0, LzLog::s_logList, LzLogLevelNull, clock())
#define LzLogError Log(__FILE__, __LINE__, 0, LzLog::s_logList, LzLogLevelError, clock())
#define LzLogFail  Log(__FILE__, __LINE__, 0, LzLog::s_logList, LzLogLevelFail, clock())
#define LzLogInfo  Log(__FILE__, __LINE__, 0, LzLog::s_logList, LzLogLevelInfo, clock())
#define LzLogDebug Log(__FILE__, __LINE__, 0, LzLog::s_logList, LzLogLevelDebug, clock())
