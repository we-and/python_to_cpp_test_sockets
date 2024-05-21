#ifndef IO_UTILS_H
#define IO_UTILS_H

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
#include "log_utils.hpp"

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cstdlib>
#include <sys/stat.h>
#include "json.hpp"
using json = nlohmann::json;


void saveJsonToFile(const json& j, const std::string& filePath) {
   Logger* logger = Logger::getInstance();
    logger->log( "saveJsonToFile"  );
    std::ofstream file(filePath);
    if (!file) {
        std::cerr << "Error opening file for writing: " << filePath << std::endl;
        return;
    }
    file << j.dump(4); // Serialize the JSON with an indentation of 4 spaces
    file.close();
    if (file.good()) {
        logger->log( "JSON data successfully saved to " + filePath );
    } else {
        std::cerr << "Error occurred during file write operation." << std::endl;
    }
}


// Function to read a JSON file and return a JSON object
json readJsonFromFile(const std::string &filePath, Logger *logger)
{
    logger->log("readJsonFromFile");
    std::ifstream file(filePath);
    json j;

    try
    {
        file >> j; // Attempt to read and parse the JSON file
    }
    catch (const json::parse_error &e)
    {
        std::cerr << "JSON parse error: " << e.what() << '\n';
        // Optionally, return or throw another exception depending on how you want to handle errors
    }
    catch (const std::ifstream::failure &e)
    {
        std::cerr << "Error opening or reading the file: " << e.what() << '\n';
        // Optionally, return or throw another exception depending on how you want to handle errors
    }

    return j; // Return the parsed JSON object or an empty object if an exception occurred
}

std::string readStringFromFile(const std::string &filePath, Logger *logger)
{
    logger->log("    readStringFromFile: " + filePath);

    try
    {
        std::ifstream file(filePath);
        std::string token;

        if (!file)
        {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        std::getline(file, token);
        file.close();

        if (!token.empty())
        {
            return token;
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "    readStringFromFile: Exception occurred: " << e.what() << std::endl;
        logger->log("    readStringFromFile: exception");
        logger->log(e.what());
    }
    return ""; // Return empty string if there was an error
}

// Function to check if the one time exists
std::pair<bool,bool> checkFileExists(const std::string &folderPath, const std::string filename, Logger *logger){
    logger->log("    checkFileExists: " + folderPath + filename);
    fs::path myFile = folderPath + filename;
    try
    {
        bool exists = fs::exists(myFile);

        if (exists)
        {
            logger->log("    checkFileExists: File exists at " + folderPath + filename);
            return {true,true};
        }
        else
        {
            logger->log("    checkFileExists: File does not exists at" + folderPath + filename);
            return {true,false};
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "    checkFileExists: Exception occurred: " << e.what() << std::endl;
        logger->log("    checkFileExists: exception");
        logger->log(e.what());
        return {false,false};
    }
    return {false,false};
}
// Function to check if the one time exists
std::pair<bool,bool> checkFileExistsAbsPath(const std::string & filepath, Logger *logger){
    logger->log("    checkFileExists: " + filepath);
    fs::path myFile = filepath;
    try
    {
        bool exists = fs::exists(myFile);

        if (exists)
        {
            logger->log("    checkFileExists: File exists at " + folderPath + filename);
            return {true,true};
        }
        else
        {
            logger->log("    checkFileExists: File does not exists at" + folderPath + filename);
            return {true,false};
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "    checkFileExists: Exception occurred: " << e.what() << std::endl;
        logger->log("    checkFileExists: exception");
        logger->log(e.what());
        return {false,false};
    }
    return {false,false};
}

// Function to check if the one time exists
bool checkEnvVarExists(const std::string &envVar, Logger *logger)
{
    // Use getenv to get the environment variable value
    const char *value = std::getenv(envVar.c_str());

    // Check if the environment variable exists
    if (value)
    {
        logger->log("    Env var " + envVar + " exists with value: " + value);
        return true;
    }
    else
    {
        logger->log("    Env var " + envVar + " does not exist.");
        return false;
    }
}


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


ConfigFile readIniFile(const std::string& iniFilename) {
   
   
   

    std::string absPath2= getAbsolutePathRelativeToExecutable(iniFilename);
    logger->log("Reading config file            :" absPath2);
    
    auto [checkIniFileExistsSuccess, iniFileExists] = checkFileExistsAbsPath(iniFilename);
    if (!checkIniFileExistsSuccess)
    {
        std::cerr << "Ini file not found at " << absPath2 << std::endl;
        logger->log("Exiting after ini config file not found");
        std::exit(EXIT_FAILURE);
        return ConfigFile();
    }
    
    
    
    
    
    std::ifstream file(absPath2);
    ConfigFile config;
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << iniFilename << std::endl;
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
#endif

