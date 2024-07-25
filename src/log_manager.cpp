#include "log_manager.hpp"

namespace SPRF {
LogManager log_manager;

static std::string last_source = "NONE";

void CustomLog(int msgType, const char* text, va_list args) {
    std::string log_type;

    switch (msgType) {
    case LOG_INFO:
        log_type = "";
        break;
    case LOG_ERROR:
        log_type = "[ERROR]: ";
        break;
    case LOG_WARNING:
        log_type = "[WARN] : ";
        break;
    case LOG_DEBUG:
        log_type = "[DEBUG]: ";
        break;
    case LOG_CONSOLE:
        log_type = " > ";
        break;
    default:
        log_type = "";
        break;
    }

    char msg_[512];

    vsnprintf(msg_, sizeof(msg_), text, args);

    std::string msg(msg_);
    std::string source;

    if (msgType == LOG_CONSOLE) {
        source = "CONSOLE";
    } else {
        source = msg.substr(0, msg.find(":"));
        if (source.size() == 0) {
            source = last_source;
        } else {
            if (source[0] == ' ') {
                source = last_source;
                log_type = "";
            }
        }
    }

    last_source = source;

    std::string out = log_type + std::string(msg);

    log_manager.log_stack.push_back(LogMessage(out, source, msgType));
}

} // namespace SPRF