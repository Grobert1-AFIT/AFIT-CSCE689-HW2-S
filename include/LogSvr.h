#pragma once
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <strings.h>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include <fstream>

class LogSvr {
    public:
        LogSvr(char *logname);
        ~LogSvr();
        void logString(char* log);

    private:
        std::fstream logfile;

};