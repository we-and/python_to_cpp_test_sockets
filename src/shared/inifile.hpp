#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <curl/curl.h>
#include <filesystem>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "configfile.hpp"

std::pair<int,ConfigFile> readIniFile(const std::string& iniFilename) {
   
   
   

    std::string absPath2= getAbsolutePathRelativeToExecutable(iniFilename);
    
    auto [checkIniFileExistsSuccess, iniFileExists] = checkFileExistsAbsPath(iniFilename);
    if (!checkIniFileExistsSuccess)
    {
        std::cerr << "Ini file not found at " << absPath2 << std::endl;
        std::exit(EXIT_FAILURE);
        return {1,ConfigFile()};
    }
    
    
    
    
    
    std::ifstream file(absPath2);
    ConfigFile config;
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << iniFilename << std::endl;
        return {1,config}; // Return default-initialized config if file opening fails
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                // Remove potential whitespace that might be around the '='
                key.erase(std::remove_if(key.begin(), key.end(), isspace), key.end());
                value.erase(std::remove_if(value.begin(), value.end(), isspace), value.end());

                if (key == "PORT") {
                    config.port = std::stoi(value);
                } else if (key == "BASE_URL") {
                    config.baseURL = value;
                }else if (key == "LOGS_DIR") {
                    config.logsDir = value;
                }else if (key == "DEVICE_SECURITY_PARAMETERS_PATH") {
                    config.deviceSecurityParametersPath = value;
                }
            }
        }
    }

    file.close();
    return {0,config};
}