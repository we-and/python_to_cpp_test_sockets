#ifndef SETUP_H
#define SETUP_H
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <stdlib.h>  
#include <sstream>
#include <curl/curl.h>
#include <filesystem>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cstdlib>
#include <sys/stat.h> 
#include "config.hpp"
#include "../configfile.hpp"

#include "accesstoken.hpp"
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

std::string getAbsolutePath(const std::string& filename) {
    std::filesystem::path path = filename;
    try {
        // Resolve to absolute path and normalize
        return std::filesystem::absolute(path).string();
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return std::string();
    }
}


ConfigFile readIniFile(const std::string& filename) {
   std::string absPath2= getAbsolutePathRelativeToExecutable(filename);
    std::cout <<"Reading config file from"<< absPath2 <<std::endl;
 //    std::string absolutePath = getAbsolutePath(filename);
   // std::cout << absolutePath <<std::endl;
    std::ifstream file(absPath2);
    ConfigFile config;
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return config; // Return default-initialized config if file opening fails
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
    return config;
}



// Function to prompt user and get a string
std::string getUserInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input); // Use getline to read the whole line
    return input;
}

std::string inputSecretToken(){
        std::cout << "Secret token does not exist. Please run set_token <YOUR_SECRET_TOKEN> beforehand or enter it below:" << std::endl;
        // Prompt for user input
        std::string userString = getUserInput("Please enter a secret token: ");
        return userString;
}





//return {0,accesstoken} if success and has a token
//return {1,""} otherwise 
std::pair<int,std::string> setup(const Config& appConfig){
    Logger* logger = Logger::getInstance();
    auto posDirectory=appConfig.getPosDirectory();
    auto secretTokenFilename=appConfig.getSecretTokenFilename();
    
    logger->log( "Setup"  );     
    auto [isSuccess,hasValidSecretToken_]=hasValidSecretToken(posDirectory,secretTokenFilename);
    if (!isSuccess){ return {1,""};}
    bool hasValidSessionToken_=hasValidSessionTokenInit();
    if(hasValidSecretToken_){
        if(hasValidSessionToken_){
            std::cout << "App setup and ready."  << std::endl;     
            const char* access_token = std::getenv("ACCESS_TOKEN");
            return {0,access_token};
        }else{
            logger->log( "Requesting access_token from secret."  );
            fs::path secretTokenPath = posDirectory+secretTokenFilename;
            std::string secretToken=readStringFromFile(secretTokenPath,logger);


            //debug
            //send sequence hash directly
                            std::cout << "Debug send hash directly" <<std::endl;

            ActivateDeviceAPIResponse response=ActivateDeviceAPIResponse();
          std::string jsonstr = std::string("{    \"deviceId\": 8,    \"deviceKey\": \"2c624232cdd221771294dfbb310aca000a0df6ac8b66b696d90ef06fdefb64a3\", \"deviceSequence\": 1}");
         std::cout << "Debug parse" <<std::endl;
            logger->log("Force activation from"+jsonstr);
           bool isValid=response.parseFromJsonString(jsonstr);
            std::cout << "Debug"<<isValid <<std::endl;
            if (isValid){
                auto [success,sessiontoken]=processActivateResponseOK(response,logger,appConfig);
                std::cout << sessiontoken <<std::endl; 
                return {success,sessiontoken};
            }else{
                 return {1,""};
            }
            //end debug
//            return requestAccessTokenFromSecretToken(secretToken,appConfig);
        }
    }else{
        logger->log( "Setup failed, no secret token"  );
        
        return {1,""};
    }
}
#endif