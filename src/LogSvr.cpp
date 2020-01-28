#include "LogSvr.h"

LogSvr::LogSvr(const char* logname):logLocation(logname) {
}

LogSvr::~LogSvr() {
}

void LogSvr::logString(std::string logString) {
    logfile.open(logLocation, std::ios_base::app);
    std::string dateTime = CurrentDate();
    logString += dateTime;
    logString += "\n";
    logfile.write(logString.c_str(), logString.length());
    logfile.flush();
    logfile.close();
}

std::string LogSvr::CurrentDate()
{
    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    char buf[100] = {0};
    std::strftime(buf, sizeof(buf), "%H:%M:%S  %Y-%m-%d", std::localtime(&now));
    return buf;
}