#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <curl/curl.h>
#include <filesystem>

#include <algorithm>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "configfile.hpp"
namespace fs = std::filesystem;


//GET PATH OF CURRENT EXECUTABLE, used to find settings.ini 
#if defined(_WIN32)
#include <windows.h>
std::string getExecutablePath() {
    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0) {
        return std::filesystem::path(path).parent_path().string();
    }
    return std::string();
}
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
std::string getExecutablePath() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::filesystem::path(path).parent_path().string();
    }
    return std::string();
}
#elif defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count > 0) {
        // Create a filesystem path from the result buffer and get the parent path
        std::filesystem::path exePath(result, result + count);
        return exePath.parent_path().string();
    }
    return std::string();  // Return an empty string if failed
}
#else
#error "Unsupported platform."
#endif


std::string getAbsolutePathRelativeToExecutable(const std::string& filename) {
    std::filesystem::path exePath = getExecutablePath();
    std::filesystem::path filePath = exePath / filename;
    try {
        // Resolve to absolute path and normalize
        return std::filesystem::absolute(filePath).string();
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return std::string();
    }
}
// Function to check if the one time exists
std::pair<bool,bool> checkFileExistsAbsPath(const std::string & filepath){
    fs::path myFile = filepath;
    try
    {
        bool exists = fs::exists(myFile);

        if (exists)
        {
            return {true,true};
        }
        else
        {
            return {true,false};
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "    checkFileExists: Exception occurred: " << e.what() << std::endl;
        return {false,false};
    }
    return {false,false};
}


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
                }else if (key == "APP_DIR") {
                    config.appDir = value;
                }else if (key == "SERVER_EXECUTABLE") {
                    config.serverExecutable = value;
                }else if (key == "DEVICE_SECURITY_PARAMETERS_PATH") {
                    config.deviceSecurityParametersPath = value;
                }
            }
        }
    }

    file.close();
    return {0,config};
}