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

// Function to read a JSON file and return a JSON object
json readJsonFromFile(const std::string& filePath) {
     log_to_file( "readJsonFromFile"); 
   std::ifstream file(filePath);
    json j;

    try {
        file >> j;  // Attempt to read and parse the JSON file
    } catch (const json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << '\n';
        // Optionally, return or throw another exception depending on how you want to handle errors
    } catch (const std::ifstream::failure& e) {
        std::cerr << "Error opening or reading the file: " << e.what() << '\n';
        // Optionally, return or throw another exception depending on how you want to handle errors
    }

    return j;  // Return the parsed JSON object or an empty object if an exception occurred
}





std::string readStringFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    std::string token;
    if (file) {
        std::getline(file, token);
        file.close();
        if (!token.empty()) {
            return token;
        }
    }
    return ""; // Return empty string if file does not exist or is empty
}


// Function to check if the one time exists
bool checkFileExists(const std::string& folderPath,const std::string filename) {
    log_to_file( "checkFileExists: "+folderPath+filename); 
    
    fs::path myFile = folderPath+filename;
    bool exists= fs::exists(myFile);

    if (exists) {
        log_to_file("checkFileExists: File exists at "+folderPath+filename);
        return true;
    } else {
        log_to_file("checkFileExists: File does not exists at"+folderPath+filename);
        return false;
    }

}

// Function to check if the one time exists
bool checkEnvVarExists(const std::string& envVar) {
   // Use getenv to get the environment variable value
    const char* value = std::getenv(envVar.c_str());

    // Check if the environment variable exists
    if (value) {
       log_to_file("Env var "+  envVar + " exists with value: " + value );
        return true;
    } else {
        log_to_file("Env var "+ envVar + " does not exist." );
        return false;
    }

}
