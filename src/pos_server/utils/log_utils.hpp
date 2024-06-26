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

std::mutex logMutex;
namespace fs = std::filesystem;

class Logger {
private:
    static Logger* instance;
    static std::mutex mutex;
    Config appConfig;  // Configuration instance as a class member
int currentDayOfWeek;
protected:
    Logger() :currentDayOfWeek(-1){}  // Constructor is protected

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
        // Get the current day of the week
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm = *std::localtime(&tt);
        char dayOfWeekStr[2]; // Day of week (0-6)
        std::strftime(dayOfWeekStr, sizeof(dayOfWeekStr), "%w", &now_tm);
        int dayOfWeek = std::stoi(dayOfWeekStr);

        if (dayOfWeek != currentDayOfWeek) {
            currentDayOfWeek = dayOfWeek;
            return true;
        }
        return false;
    }



std::string getFilePath(){
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);



    auto filePath=appConfig.logsDir + "/log-" + std::to_string(t) +"-"+std::to_string(currentDayOfWeek)+ ".txt";
    return filePath;
}
    // Initialization method for setting up the configuration
    void init(const Config& appConfig_) {
        appConfig = appConfig_;
          auto filePath=getFilePath();
        std::cout <<"Log file               :"<< filePath <<std::endl;

    }
    void log(const int& text) {
        return log(std::to_string(text));
    }
    /*
    bool shouldRotateLogFile() {
        auto now = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::hours>(now - t).count();
        return duration >= 168;  // 7 days * 24 hours = 168 hours
    }*/

    void log(const std::string& text) {

         logStream << "Thread " << std::this_thread::get_id() << ": " << message;

    // Lock the mutex to ensure thread-safe console output
        std::lock_guard<std::mutex> guard(logMutex);


        std::ios_base::openmode mode;
         if (shouldRotateLogFile()) {//overwrite if new day 
               mode= std::ios::trunc;
        }else{           //otherwise append to file
           mode= std::ios::app;
        }


        // Retrieve the current system time as a time_t object
        auto now = std::chrono::system_clock::now();
        auto tt = std::chrono::system_clock::to_time_t(now);

        auto filePath=getFilePath();
        // Create or open a log file named with the current time stamp
        std::ofstream log_file(filePath, std::ios::app);

        // Check if the file was successfully opened
        if (!log_file.is_open()) {
            std::cerr << "Failed to open log file at " <<filePath<< std::endl;
            return;
        }

        // Write the current time and the log message to the file
        log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << "Thread " << std::this_thread::get_id() << ": " << text << "\n";
    }
};

#endif