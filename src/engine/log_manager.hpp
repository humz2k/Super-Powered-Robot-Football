#ifndef _SPRF_LOG_MANAGER_HPP_
#define _SPRF_LOG_MANAGER_HPP_

#include "raylib-cpp.hpp"
#include <string>

#define LOG_CONSOLE 10

namespace SPRF {

void CustomLog(int msgType, const char* text, va_list args);

struct LogMessage {
    std::string message;
    std::string source;
    int type;
    LogMessage(std::string message_, std::string source_, int type_)
        : message(message_), source(source_), type(type_) {}
};

class LogManager {
  public:
    std::vector<LogMessage> log_stack;

    LogManager() { SetTraceLogCallback(CustomLog); }
};

extern LogManager log_manager;

} // namespace SPRF

#endif // _SPRF_LOG_MANAGER_HPP_