#include "LogSvr.h"

LogSvr::LogSvr(char* logname) {
    logfile.open(logname, std::fstream::app);
    if (!logfile) {
    std::cerr << "Unable to read log file\n";
   }
}

LogSvr::~LogSvr() {
    logfile.close();
}

void LogSvr::logString(char* logString) {
    logfile.write(logString, sizeof(logString));
}