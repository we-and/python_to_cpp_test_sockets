#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>

#include <thread>
#include <sstream>
#include <mutex>
#include <stdlib.h>
#include <sstream>
#include <curl/curl.h>
#include <filesystem>
#include "json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cstdlib>
#include <sys/stat.h>
#include "config.hpp"

// Retrieve the current system time as a time_t object
auto serverStartTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

std::mutex logMutex;
namespace fs = std::filesystem;

class Logger
{
private:
    static Logger *instance;
    static std::mutex mutex;
    Config appConfig; // Configuration instance as a class member
    int currentDayOfWeek;

bool isDebugPeriod=true;
  int currentLogPeriod = -1;
    std::string logFilepath;
    std::string logsDir ;
protected:
    Logger() : currentDayOfWeek(-1) {} // Constructor is protected

public:
    // Prevent copying
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;


    void setLogFilename(){
        if (isDebugPeriod){
            setLogFilenameFrequentRotations();
        }else{
            setLogFilenameOnceADay();
        }
    }
    void setLogFilenameOnceADay(){
        logFilepath=  logsDir + "/log-" + std::to_string(serverStartTime) + "-" + std::to_string(currentDayOfWeek) + ".txt";    

    }
     bool shouldRotateLogFrequentRotations() {
        auto now = std::chrono::system_clock::now();
         auto nowTimeT = std::chrono::system_clock::to_time_t(now);
        auto elapsed = std::difftime(nowTimeT, serverStartTime) / 60;//convert to minutes

        //auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - serverStartTime).count();
        int currentPeriod = elapsed / 3;
        if (currentPeriod != currentLogPeriod) {
            currentLogPeriod = currentPeriod;
            log("shouldRotateLogFrequentRotations yes, period=" +std::to_string(currentPeriod));
            return true;
        }
        log("shouldRotateLogFrequentRotations no, period=" +std::to_string(currentPeriod));

        return false;
    }


    void setLogFilenameFrequentRotations() {
          log("setLogFilenameFrequentRotations ");
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << logsDir << "/log-" << timestamp << "-" << currentLogPeriod << ".txt";
        logFilepath = ss.str();
                  log("setLogFilenameFrequentRotations set"+logFilepath);

    }
    static Logger *getInstance()
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr)
        {
            instance = new Logger();
        }
        return instance;
    }
    void setDayOfTheWeek()
    {
        // Get the current day of the week
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&tt);
        char dayOfWeekStr[2]; // Day of week (0-6)
        std::strftime(dayOfWeekStr, sizeof(dayOfWeekStr), "%w", &now_tm);
        currentDayOfWeek = std::stoi(dayOfWeekStr);
    }

    bool shouldRotateLogFile(){
        if (isDebugPeriod){
           return shouldRotateLogFrequentRotations();
        }else{
           return shouldRotateLogFileOnceADay();
        }
    }
    bool shouldRotateLogFileOnceADay()
    {
        // Get the current day of the week
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&tt);
        char dayOfWeekStr[2]; // Day of week (0-6)
        std::strftime(dayOfWeekStr, sizeof(dayOfWeekStr), "%w", &now_tm);
        int dayOfWeek = std::stoi(dayOfWeekStr);

        if (dayOfWeek != currentDayOfWeek)
        {
            currentDayOfWeek = dayOfWeek;
            setLogFilename();
            return true;
        }
        return false;
    }
    void deleteOldLogsOldFormat()
    {
        auto now = std::chrono::system_clock::now();
        auto oneWeekAgo = now - std::chrono::hours(24 * 7);

        for (const auto &entry : fs::directory_iterator(appConfig.logsDir))
        {
            if (entry.is_regular_file())
            {
                std::string filePath = entry.path().string();
                std::string filename = entry.path().filename().string();

                // Check if the filename starts with "log-" and ends with ".txt"
                if (filename.find("log-") == 0 && filename.rfind(".txt") == filename.length() - 4)
                {
                    // Extract the timestamp from the filename
                    std::string timeStr = filename.substr(4, filename.length() - 8);
                    try
                    {
                        std::time_t fileTime = std::stoll(timeStr);
                        auto fileTimePoint = std::chrono::system_clock::from_time_t(fileTime);

                        if (fileTimePoint < oneWeekAgo)
                        {
                            fs::remove(entry.path());
                            log("Deleted old log file: " + filePath);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cerr << "Error parsing time from filename: " << filename << " (" << e.what() << ")" << std::endl;
                    }
                }
            }
        }
    }
    void deleteOldLogs()
    {
                       log("Deleting old log files ");
         
         int nDeleted=0;
        deleteOldLogsOldFormat();
        auto now = std::chrono::system_clock::now();
        auto oneWeekAgo = now - std::chrono::hours(24 * 7);

        for (const auto &entry : fs::directory_iterator(appConfig.logsDir))
        {
            if (entry.is_regular_file())
            {
                std::string filePath = entry.path().string();
                std::string filename = entry.path().filename().string();

                // Extract the timestamp from the filename
                size_t pos1 = filename.find("log-");
                size_t pos2 = filename.find("-", pos1 + 4);
                if (pos1 == std::string::npos || pos2 == std::string::npos)
                {
                    continue; // Skip files that don't match the expected format
                }

                std::string timeStr = filename.substr(pos1 + 4, pos2 - (pos1 + 4));
                try
                {
                    std::time_t fileTime = std::stoll(timeStr);
                    auto fileTimePoint = std::chrono::system_clock::from_time_t(fileTime);

                    if (fileTimePoint < oneWeekAgo)
                    {
                        nDeleted++;         
                        fs::remove(entry.path());
                        log("Deleted old log file: " + filePath);
                    }else{
                        log("Log file: " + filePath+" kept as recent");
            
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error parsing time from filename: " << filename << " (" << e.what() << ")" << std::endl;
                }
            }
        }
    }

    std::string getFilePath()
    {
        return logFilepath;
/*
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);

        auto filePath = appConfig.logsDir + "/log-" + std::to_string(serverStartTime) + "-" + std::to_string(currentDayOfWeek) + ".txt";
        return filePath;
  */
    }
    // Initialization method for setting up the configuration
    void init(const Config &appConfig_)
    {
        appConfig = appConfig_;
        setDayOfTheWeek();
        setLogFilename();
        //auto filePath = getFilePath();
        std::cout << "Rotated log file               : " << filePath << std::endl;
        
        deleteOldLogs();
    }
    void log(const int &text)
    {
        return log(std::to_string(text));
    }

    void log(const std::string &text)
    {

        // Lock the mutex to ensure thread-safe console output
        std::lock_guard<std::mutex> guard(logMutex);

        std::ios_base::openmode mode;
        if (shouldRotateLogFile())
        { // overwrite if new day
            mode = std::ios::trunc;
            deleteOldLogs();
        }
        else
        { // otherwise append to file
            mode = std::ios::app;
        }

        // Retrieve the current system time as a time_t object
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);

        auto filePath = getFilePath();
        // Create or open a log file named with the current time stamp
        std::ofstream log_file(filePath, std::ios::app);

        // Check if the file was successfully opened
        if (!log_file.is_open())
        {
            std::cerr << "Failed to open log file at " << filePath << std::endl;
            return;
        }

        // Write the current time and the log message to the file
        auto tid = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << "Thread " << tid << ": " << text << "\n";
    }
};

#endif