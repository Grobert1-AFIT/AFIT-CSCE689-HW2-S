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
#include <chrono>

class LogSvr {
    public:
        LogSvr(const char* logname);
        ~LogSvr();
        void logString(std::string log);
        std::string CurrentDate();

    private:
        std::ofstream logfile;
        std::string logLocation;

};