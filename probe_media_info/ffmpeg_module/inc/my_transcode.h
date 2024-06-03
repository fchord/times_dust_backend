#pragma once
#include <string>
#include <future>

int mt_init(std::string &mt_config);
std::future<int> &work();