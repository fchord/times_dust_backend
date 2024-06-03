#pragma once
#include <string>

using namespace std;

class IListener
{
public:
	virtual void OnProgressReport(float progress = 0.0) = 0;
	virtual void OnFrameReport(void *frame = NULL) = 0;
	virtual void OnErrorReport(std::string err = std::string("")) = 0;
};