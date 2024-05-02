#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
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

// Retrieve the current system time as a time_t object
auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());


namespace fs = std::filesystem;

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
 */
using json = nlohmann::json;
void log_to_file(const std::string& text) {

    // Create or open a log file named with the current time stamp
    std::ofstream log_file("log-" + std::to_string(t) + ".txt", std::ios::app);


    // Retrieve the current system time as a time_t object
    auto tt = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Write the current time and the log message to the file
    log_file << std::put_time(std::localtime(&tt), "%F %T") << " - " << text << "\n";
}


#endif