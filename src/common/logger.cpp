#include <common/logger.hpp>
#include <cstdarg>
#include <mutex>

using namespace familyline;
using namespace std::chrono;

std::unique_ptr<Logger> LoggerService::_logger;

std::mutex mtx;

void Logger::write(std::string_view tag, LogType type, const char* format, ...)
{
    if (type < _minlog) return;
    if (std::find(blockTags_.begin(), blockTags_.end(), tag) != blockTags_.end()) return;

    const char* prefix;
    const char* colorstart;
    const char* colorend = "\033[0m";

    switch (type) {
        case LogType::Debug:
            prefix     = "[debug] ";
            colorstart = "\033[3m";
            break;
        case LogType::Info:
            prefix     = "";
            colorstart = "";
            break;
        case LogType::Warning:
            prefix     = "[warn] ";
            colorstart = "\033[33m";
            break;
        case LogType::Error:
            prefix     = "[error] ";
            colorstart = "\033[31m";
            break;
        case LogType::Fatal:
            prefix     = "[fatal] ";
            colorstart = "\033[31;1m";
            break;
        default: prefix = ""; break;
    }

    char msg[1024] = {};

    va_list vl;
    va_start(vl, format);

    vsnprintf(msg, 512, format, vl);

    va_end(vl);

    mtx.lock();

    if (tag == "") {
        fprintf(_out, "[%13.4f] %s%s%s%s\n", this->getDelta(), prefix, colorstart, msg, colorend);
    } else {
        char stag[128] = {};
        snprintf(stag, 127, "\033[1m%s\033[0m", tag.data());

        fprintf(
            _out, "[%13.4f] %s%s: %s%s%s\n", this->getDelta(), prefix, stag, colorstart, msg,
            colorend);
    }

    mtx.unlock();
}

double Logger::getDelta()
{
    decltype(_start) now = steady_clock::now();
    auto deltatime       = duration_cast<duration<double>>(now - _start);
    return deltatime.count();
}
