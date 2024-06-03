#pragma once
#include <iostream>
#include <string>
#include <boost/json.hpp>

// using namespace boost::json;

class CMyConfig
{
public:
    static CMyConfig *GetInstance();
    int ParseInputParameter(std::string input_parameter, boost::json::object &transcode_parameter);

private:
    CMyConfig();
    ~CMyConfig();
    static CMyConfig *instance;
    // static const std::string config_file;
};
