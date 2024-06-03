#include <spdlog/spdlog.h>

#if 0

class MySpdLog {
public:
    MySpdLog();
    ~MySpdLog();

    operator<<(const char* p) {

        return this;
    }
}

    spdlog::warn("Easy padding in numbers like {:08d}", 12);
    spdlog::critical("Support for int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    spdlog::info("Support for floats {:03.2f}", 1.23456);
    spdlog::info("Positional args are {1} {0}..", "too", "supported");
    spdlog::info("{:>8} aligned, {:<8} aligned", "right", "left");


#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

class Logger {
public:
    static void Init() {
        spdlog::set_pattern("%^[%T] %n: %v%$");
        s_Logger = spdlog::stdout_color_mt("APP");
        s_Logger->set_level(spdlog::level::trace);
    }

    inline static std::shared_ptr<spdlog::logger>& GetLogger() { return s_Logger; }

private:
    static std::shared_ptr<spdlog::logger> s_Logger;
};

std::shared_ptr<spdlog::logger> Logger::s_Logger;

template<typename OStream>
inline OStream& operator<<(OStream& os, const spdlog::details::line_logger& line) {
    return os << line.str();
}

#define LOG_TRACE(...) ::Logger::GetLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) ::Logger::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::Logger::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Logger::GetLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::GetLogger()->critical(__VA_ARGS__)

int main() {
    Logger::Init();
    LOG_INFO("Hello, {}!", "World");
    return 0;
}

#endif