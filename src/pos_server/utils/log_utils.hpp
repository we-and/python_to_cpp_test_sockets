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

    bool isDebugPeriod = true;
    int currentLogPeriod = 0;
    std::string logFilepath;
    std::string logsDir;
    bool isReady = false;
    int rotate_every_x_minutes= 1440;

    
protected:
    Logger() : currentDayOfWeek(-1) {} // Constructor is protected

public:
    // Prevent copying
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    void setLogFilename()
    {
        std::cout << "setLogFilename              : " << std::endl;

        if (isDebugPeriod)
        {
            setLogFilenameFrequentRotations();
        }
        else
        {
            setLogFilenameOnceADay();
        }
    }
    int calculateMaxLogFilesPerWeek(int rotationIntervalMinutes) {
    const int minutesPerDay = 24 * 60;
    const int minutesPerWeek = 7 * minutesPerDay;

    // Calculate the number of intervals in a week
    int maxLogFiles = minutesPerWeek / rotationIntervalMinutes;

    return maxLogFiles;
}
    void setLogFilenameOnceADay()
    {
        logFilepath = logsDir + "/log-" + std::to_string(serverStartTime) + "-" + std::to_string(currentDayOfWeek) + ".txt";
    }
    bool shouldRotateLogFrequentRotations()
    {
        auto now = std::chrono::system_clock::now();
        auto nowTimeT = std::chrono::system_clock::to_time_t(now);
        auto elapsed_sec = std::difftime(nowTimeT, serverStartTime);
        auto elapsed = elapsed_sec / 60; // convert to minutes

        // auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - serverStartTime).count();
        int currentPeriod = elapsed / rotate_every_x_minutes;
        if (currentPeriod != currentLogPeriod)
        {
            int maxPerPeriod=calculateMaxLogFilesPerWeek(rotate_every_x_minutes);
            if (currentPeriod>=maxPerPeriod){
                currentPeriod=0;
            }
            currentLogPeriod = currentPeriod;


            //      std::cout << ("shouldRotateLogFrequentRotations yes, period=" + std::to_string(currentPeriod))<<std::endl;
            logSimple("shouldRotateLogFrequentRotations yes, period=" + std::to_string(currentPeriod));
            return true;
        }
        //        std::cout <<("shouldRotateLogFrequentRotations no, period=" + std::to_string(currentPeriod))<<std::endl;;
        //logSimple("shouldRotateLogFrequentRotations no, nowtimet=" + std::to_string(nowTimeT) + " serverstart=" + std::to_string(serverStartTime) + " compperiod=" + std::to_string(currentPeriod) + " logperiod=" + std::to_string(currentPeriod) + " ela=" + std::to_string(elapsed_sec));

        return false;
    }

    void setLogFilenameFrequentRotations()
    {
        std::cout << "setLogFilenameFrequentRotations              : " << std::endl;

        //          log("setLogFilenameFrequentRotations ");
        //auto now = std::chrono::system_clock::now();
        //auto timestamp = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << logsDir << "/log-" << serverStartTime << "-" << currentLogPeriod << ".txt";
        logFilepath = ss.str();
        std::cout << "setLogFilenameFrequentRotations filepath=             : " << logFilepath << std::endl;
        //    log("setLogFilenameFrequentRotations set"+logFilepath);
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

    bool shouldRotateLogFile()
    {
        if (isDebugPeriod)
        {
            return shouldRotateLogFrequentRotations();
        }
        else
        {
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
                            std::cout << ("Deleted old log file (old format): " + filePath) << std::endl;

                            logSimple("Deleted old log file: " + filePath);
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
        logSimple("Deleting old log files ");

        int nDeleted = 0;
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
                        //                        std::cout <<("Deleted old log file: " + filePath) <<std::endl;
                        logSimple("Deleted old log file: " + filePath);
                    }
                    else
                    {
                        std::cout << ("Log file: " + filePath + " kept as recent") << std::endl;
                        logSimple("Log file: " + filePath + " kept as recent");
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
        std::cout << "[LOG] Init" << std::endl;

        appConfig = appConfig_;
        logsDir = appConfig.logsDir;
        setDayOfTheWeek();
        setLogFilename();
        // auto filePath = getFilePath();

        isReady = true;
        logSimple("[LOG] Log ready");
        logSimple("[LOG] Rotated log file               : " + logFilepath);

        deleteOldLogs();
        logSimple("[LOG] Init finished ");
    }
    void log(const int &text)
    {
        if (!isReady)
        {
            std::cout << text << std::endl;
            return;
        }
        return log(std::to_string(text));
    }

    void logSimple(const std::string &text)
    {
        std::cout << "LOGS" << isReady << " " << text << std::endl;

        if (!isReady)
        {
            std::cout << text << std::endl;
            return;
        }
        std::cout << "LOGS" << isReady << " AA" << std::endl;
        // Lock the mutex to ensure thread-safe console output
        //        std::lock_guard<std::mutex> guard(logMutex);

        std::cout << "LOGS" << isReady << " A" << std::endl;

        // Retrieve the current system time as a time_t object
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        auto tid = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        auto filePath = getFilePath();
        // Create or open a log file named with the current time stamp
        std::ofstream log_file(filePath, std::ios::app);
        std::cout << "LOGS" << isReady << " B" << std::endl;

        // Check if the file was successfully opened
        if (!log_file.is_open())
        {
            std::cerr << "Failed to open log file at " << filePath << std::endl;
            return;
        }

        // Write the current time and the log message to the file
        std::cout << "LOGS" << isReady << " C" << std::endl;

        log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << "Thread " << tid << ": " << text << "\n";
        std::cout << "LOGS" << isReady << " D" << std::endl;
    }
    void log(const std::string &text)
    {

        std::cout << "LOG" << isReady << " " << text << std::endl;

        if (!isReady)
        {
            std::cout << text << std::endl;
            return;
        }
        // Lock the mutex to ensure thread-safe console output
        std::lock_guard<std::mutex> guard(logMutex);

        std::ios_base::openmode mode;

        //        logSimple("L Should rotate");
        bool shouldRotate = shouldRotateLogFile();
        //      logSimple("L Should rotate "+std::string(shouldRotate?"yes":"no"));
        if (shouldRotate)
        { // overwrite if new day

            mode = std::ios::trunc;
            deleteOldLogs();
            setLogFilename();
        }
        else
        { // otherwise append to file
            mode = std::ios::app;
        }
        // logSimple("L log 1");

        // Retrieve the current system time as a time_t object
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        auto tid = std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        auto filePath = getFilePath();
        // logSimple("L log 2");
        std::cout << "LOG now=" << filePath << std::endl;
        // Create or open a log file named with the current time stamp
        std::ofstream log_file(filePath, std::ios::app);

        // Check if the file was successfully opened
        if (!log_file.is_open())
        {
            std::cerr << "Failed to open log file at " << filePath << std::endl;
            return;
        }
        std::cout << "LOG now=" << std::put_time(std::localtime(&tt), "%F %T") << " - " << "Thread " << tid << ": " << text << "\n";

        // Write the current time and the log message to the file

        log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << "Thread " << tid << ": " << text << "\n";
    }
};

#endif