#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>

#include <ctime>
#include <string>


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
auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());


namespace fs = std::filesystem;

class Logger {
private:
    static Logger* instance;
    static std::mutex mutex;
     std::chrono::system_clock::time_point logStartTime;
    Config appConfig;  // Configuration instance as a class member
    std::string initialDate;
  int initialWeekNumber;
protected:
    Logger() {}  // Constructor is protected

public:
    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger* getInstance() {
        std::lock_guard<std::mutex> lock(mutex);
        if (instance == nullptr) {
            instance = new Logger();
        }
        return instance;
    }
    bool shouldRotateLogFile() {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - logStartTime).count();
        return duration >= 168;  // 7 days * 24 hours = 168 hours
    }

    void rotateLogFile() {
        logStartTime = std::chrono::system_clock::now();
    }
      std::string getFilePath() {
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        
                // Get the current week number
        std::tm now_tm = *std::localtime(&tt);
        char weekNumberStr[3]; // Week number can be two digits
        std::strftime(weekNumberStr, sizeof(weekNumberStr), "%U", &now_tm);
        int currentWeekNumber = std::stoi(weekNumberStr);

        return appConfig.logsDir + "/log-" + initialDate + "-" + std::to_string(initialWeekNumber + (currentWeekNumber - initialWeekNumber)) + ".txt";
  


    }

    // Initialization method for setting up the configuration
    void init(const Config& appConfig_) {
        appConfig = appConfig_;
          auto filePath=getFilePath();
           logStartTime = std::chrono::system_clock::now();

            // Get the initial date and week number
        auto tt = std::chrono::system_clock::to_time_t(logStartTime);
        auto now_tm = *std::localtime(&tt);

  char dateStr[11]; // Date in YYYY-MM-DD format
        std::strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &now_tm);
        initialDate = dateStr;

        char weekNumberStr[3]; // Week number can be two digits
        std::strftime(weekNumberStr, sizeof(weekNumberStr), "%U", &now_tm);
        initialWeekNumber = std::stoi(weekNumberStr);

          rotateLogFile();

        std::cout <<"Log file               :"<< filePath <<std::endl;
    }
    void log(const int& text) {
        return log(std::to_string(text));
    }

    void log(const std::string& text) {
          if (shouldRotateLogFile()) {
            rotateLogFile();
        }

        // Retrieve the current system time as a time_t object
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);

        auto filePath=appConfig.logsDir + "/log-" + std::to_string(t) + ".txt";
        // Create or open a log file named with the current time stamp
        std::ofstream log_file(filePath, std::ios::app);

        // Check if the file was successfully opened
        if (!log_file.is_open()) {
            std::cerr << "Failed to open log file at " <<filePath<< std::endl;
            return;
        }

        // Write the current time and the log message to the file
        log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << text << "\n";
    }
};
#endif