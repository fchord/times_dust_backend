#include <iostream>
#include <string>
#include "lz_utils.h"
#include "lz_log.h"

#ifdef FFMPEG_LOG
extern "C"
{
#include "libavformat/avformat.h"
#include "libavformat/avio.h"
#include "libavformat/version.h"
#include "libavfilter/avfilter.h"
#include "libavdevice/avdevice.h"
}
#endif

using namespace std;



Log::Log(Log& log) {
	strSrc = log.strSrc;
	lineNumber = log.lineNumber;
	threadId = log.threadId;
	level = log.level;
	clock = log.clock;
	strLog = log.strLog;
	pList = NULL;
	m_toPush = false; // IF THIS IS A COPIED LOG, NOT NEED TO PUSH INTO LIST ANYMORE.
	return;
}

Log::Log(const char *src, const int line, unsigned int tid, LogList* loglist, LzLogLevel lvl, clock_t clk) {
	strSrc.append(src);
	lineNumber = line;
	threadId = tid;
	pList = loglist;
	level = lvl;
	clock = clk;
	while (loglist && loglist->length() >= 4 * 1024 * 1024)
	{
		//std::cout << "loglist length: " << loglist->length() << std::endl;
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
	return;
}

Log::~Log() {
	if (pList && m_toPush) {
		m_toPush = false;
		Log* log = new Log(*this);
		pList->push(log);
	}
	return;
};

wofstream LzLog::fsLog;

wstring LzLog::sm_wsPath;

LzLog* LzLog::sm_pLzLog;

LogList* LzLog::s_logList;

void LzLog::Init(wstring path)
{
	time_t rawtime;
	struct tm * timeinfo;
	stringstream ss;
	string sTime;

	sm_wsPath = path;
	if (!sm_wsPath.empty()) {
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		ss << asctime(timeinfo);
		sTime = ss.str();
		// cout << "sTime: " << sTime << endl;
		for (auto&& t : sTime) {
			if (t == ' ' || t == '\n')
				t = '_';
			if (t == ':')
				t = '-';
		}
		wstring wsLogFile = sm_wsPath + string2wstring(string("LzLog_") + sTime + string(".log"));
		wcout << L"LzLog: " << wsLogFile << endl;
		fsLog.open(wstring2string(wsLogFile));
		if (!fsLog) {
			cout << "LzLog fail: " << fsLog.fail() << endl;
		}
	}
	s_logList = new LogList();
	sm_pLzLog = new LzLog();
	return;
}

void LzLog::popLogList()
{
	LogList loglist;
	std::array<Log*, arrLogSize> arrLogs;
	int arrLen = 0, clen;
	// std::string s;
	const int csize = 1024, ccsize = 16 * 1024;
	int cclen = 0;
	char* cc = NULL;
	char c[csize];
	clock_t c0, c1, c2, c3;
	int cc0 = 0, cc1 = 0, cc2 = 0;

	memset(c, 0, csize);
	cc = new char[ccsize];
	memset(cc, 0, ccsize);

	while (1)
	{
		if (bQuit && 0 == s_logList->length())
		{
			m_mtxBufs.lock();
			m_ltBufs.push_back(cc);
			m_mtxBufs.unlock();
			cc = NULL;
			break;
		}
		if (0 == s_logList->length()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		//c0 = clock();
		arrLen = s_logList->popLogs(arrLogs);
		//std::cout << "arrLen: " << arrLen << std::endl;
		//cc0 += clock() - c0;
		//std::cout << "c0: " << clock() - c0 << std::endl;
		for (int i = 0; i < arrLen; i++) {
			// c1 = clock();
			Log* log = arrLogs[i];
			//c = new char[csize];			
#ifdef LOG_TIMESTAMP
				/*int hour, minite, second, msecond;
				/*ss << std::setw(2) << std::setfill('0') << hour << ":"
					<< std::setw(2) << std::setfill('0') << minite << ":"
					<< std::setw(2) << std::setfill('0') << second << "."
					<< std::setw(3) << std::setfill('0') << msecond << " ";*/
			clen = strlen(c);
			snprintf(c + clen, csize - clen, "%02d:%02d:%02d.%03d ",
				(log->clock / (1000 * 60 * 60)) % 100,
				(log->clock / (1000 * 60)) % 60,
				(log->clock / 1000) % 60,
				log->clock % 1000);
#endif
			if (log->level == LzLogLevelError) 
				strcat(c, "LZ_LOG_ERROR ");
			else if (log->level == LzLogLevelFail) 
				strcat(c, "LZ_LOG_FAIL  ");
			else if (log->level == LzLogLevelInfo) 
				strcat(c, "LZ_LOG_INFO  ");
			else if (log->level == LzLogLevelDebug) 
				strcat(c, "LZ_LOG_DEBUG ");

			if(log->level != LzLogLevelNull)
			{
	#ifdef SOURCE_WITH_PATH
				clen = strlen(c);
				snprintf(c + clen, csize - clen, "[%s", log->strSrc.c_str());
	#else
				std::string src;
				int n = log->strSrc.length() - 1;
				while (log->strSrc[n] != '\\' && n > 0)
					n--;
				n++;
				src = log->strSrc.substr(n, log->strSrc.length() - n);
				clen = strlen(c);
				snprintf(c + clen, csize - clen, "[%s", src.c_str());
	#endif
				//std::stringstream ssId;
				//ssId << log.threadId;
				//_Thrd_t t = *(_Thrd_t*)(char*)&log.threadId;
				clen = strlen(c);
				snprintf(c + clen, csize - clen, ":%d. %u] ", log->lineNumber, log->threadId);
		}
			else {
				clen = strlen(c);
				snprintf(c + clen, csize - clen, "[%u] ", log->threadId);
			}
			clen = strlen(c);
			strncat(c, log->strLog.c_str(), csize - clen - 1);
			strcat(c, "\n");
			delete log;
			log = NULL;
			//c2 = clock();
			//cc1 += c2 - c1;

			clen = strlen(c);
			if (ccsize - cclen <= clen)
			{
				while (1)
				{
					m_mtxBufs.lock();
					if (m_ltBufs.size() < 64)
					{
						m_mtxBufs.unlock();
						break;
					}
					m_mtxBufs.unlock();
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
				}
				m_mtxBufs.lock();
				m_ltBufs.push_back(cc);
				m_mtxBufs.unlock();
				cc = new char[ccsize];
				memset(cc, 0, ccsize);
				cclen = 0;
			}
			strcat(cc, c);
			cclen += clen;
			memset(c, 0, csize);
		}
	}
	//std::cout << "cc1: " << cc1 << ", cc2: " << cc2 << std::endl;
	//std::cout << "cc0: " << cc0 << std::endl;
	if (cc)
		delete cc;
	bPopQuit = true;
	return;
}

void LzLog::flushLog()
{
	char* c = NULL;
	const int iArrSize = 64;
	std::array<char*, iArrSize> arr;
	int size = 0;
	while (1) {
		if (bQuit && bPopQuit)
		{
			m_mtxBufs.lock();
			while (m_ltBufs.size() != 0)
			{
				c = m_ltBufs.front();
				m_ltBufs.pop_front();
				fsLog << c;
				delete c;
				c = NULL;
			}
			m_mtxBufs.unlock();
			break;
		}
		m_mtxBufs.lock();
		if (m_ltBufs.size() == 0)
		{
			m_mtxBufs.unlock();
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}
		for (size = 0; size < iArrSize && m_ltBufs.size() > 0; size++)
		{
			arr[size] = m_ltBufs.front();
			m_ltBufs.pop_front();
		}
		m_mtxBufs.unlock();
		for (int n = 0; n < size; n++)
		{
			fsLog << arr[n];
			delete arr[n];
		}
		size = 0;
	}
	fsLog.flush();
	return;
}



void LzLog::Uninit()
{
	if (sm_pLzLog) {
		delete sm_pLzLog;
		sm_pLzLog = NULL;
	}
	if (s_logList) {
		delete s_logList;
		s_logList = NULL;
	}
	fsLog.close();
	return;
}




void ffmpeglog(void* avcl, int level, const char* fmt, va_list vl)
{
#ifdef FFMPEG_LOG
	const int size = 1024;
	static int flag = av_log_get_flags();
	static char s_cache[size];
	char s[size], out[size];
	if (level > flag)
		return;

	memset(s, 0, size);
	vsnprintf(s, size - 1, fmt, vl);
	/* Maybe the end char is not '\n'. It means the line not finish. */
	if (strlen(s) > 0 && s[strlen(s) - 1] != '\n') {
		snprintf(s_cache + strlen(s_cache), size - strlen(s_cache) - 1, s);
		return;
	}

	memset(out, 0, size);
	switch (level)
	{
	case AV_LOG_QUIET:
		snprintf(out, size - 1, "AV_LOG_QUIET. ");
		break;
	case AV_LOG_PANIC:
		snprintf(out, size - 1, "AV_LOG_PANIC. ");
		break;
	case AV_LOG_FATAL:
		snprintf(out, size - 1, "AV_LOG_FATAL. ");
		break;
	case AV_LOG_ERROR:
		snprintf(out, size - 1, "AV_LOG_ERROR. ");
		break;
	case AV_LOG_WARNING:
		snprintf(out, size - 1, "AV_LOG_WARNING. ");
		break;
	case AV_LOG_INFO:
		snprintf(out, size - 1, "AV_LOG_INFO. ");
		break;
	case AV_LOG_VERBOSE:
		snprintf(out, size - 1, "AV_LOG_VERBOSE. ");
		break;
	case AV_LOG_DEBUG:
		snprintf(out, size - 1, "AV_LOG_DEBUG. ");
		break;
	case AV_LOG_TRACE:
		snprintf(out, size - 1, "AV_LOG_TRACE. ");
		break;
	default:
		break;
	}
	if (strlen(s_cache) > 0) {
		snprintf(out + strlen(out), size - strlen(out) - 1, s_cache);
		memset(s_cache, 0, size);
	}
	snprintf(out + strlen(out), size - strlen(out) - 1, s);
	if (strlen(out) > 0) {
		out[strlen(out) - 1] = '\0';
	}
	LzLogNull << out;
#endif
	return;
}

