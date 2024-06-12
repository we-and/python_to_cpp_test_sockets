#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
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
    Config appConfig;  // Configuration instance as a class member

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
std::string getFilePath(const Config& appConfig){
 auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        
        // Get the current week number
        std::tm now_tm = *std::localtime(&tt);
        char weekNumberStr[3]; // Week number can be two digits
        std::strftime(weekNumberStr, sizeof(weekNumberStr), "%U", &now_tm);
        int currentWeekNumber = std::stoi(weekNumberStr);


      auto filePath=appConfig.logsDir + "/log-" + std::to_string(t) +"- "+std::to_string(currentWeekNumber) ".txt";
      return filePath;
}
    // Initialization method for setting up the configuration
    void init(const Config& appConfig_) {
        appConfig = appConfig_;
          auto filePath=getFilePath(appConfig_);
        std::cout <<"Log file               :"<< filePath <<std::endl;

    }
    void log(const int& text) {
        return log(std::to_string(text));
    }
    bool shouldRotateLogFile() {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - t).count();
        return duration >= 168;  // 7 days * 24 hours = 168 hours
    }

    void log(const std::string& text) {
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

/**
 * Logs a message to a uniquely named file with a timestamp.
 * 
 * This function takes a string message and logs it to a text file. The filename is uniquely generated based on the current system time.
 * Each log entry in the file is prefixed with a timestamp in the "YYYY-MM-DD HH:MM:SS" format, followed by the message itself.
 * 
 * @param text The message to be logged. It should be a string containing the text that needs to be recorded in the log file.
 * 
 * Usage Example:
 * log_to_file("Application has started.");
 * 
 * Output File Example: log-1700819387.txt
 * Contents of log-1700819387.txt: 2024-04-30 12:34:56 - Application has started.

using json = nlohmann::json;
void log_to_file(const std::string& text) {

    // Create or open a log file named with the current time stamp
    std::ofstream log_file("log-" + std::to_string(t) + ".txt", std::ios::app);


    // Retrieve the current system time as a time_t object
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Write the current time and the log message to the file
    log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << text << "\n";
}
*/

#endif