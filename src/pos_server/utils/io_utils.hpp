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

bool isFullPath(const std::string& path) {
    // Check if path contains any directory delimiters
    return path.find('/') != std::string::npos || path.find('\\') != std::string::npos;
}



// Function to prompt user and get a string
std::string getUserInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input); // Use getline to read the whole line
    return input;
}
#endif

