    #include <iostream>
    #include <fstream>
    #include <chrono>
    #include <string>
    #include <stdlib.h>  
    #include <sstream>
    #include <filesystem>


#include "utils/configfile.hpp"
#include "utils/log_utils.hpp"
#include "utils/io_utils.hpp" 
#include "config.hpp"
#include "logic/setup.hpp"
#include "logic/accesstoken.hpp"
#include "logic/server.hpp"
namespace fs = std::filesystem;
using json = nlohmann::json;


// Initialize static members
Logger* Logger::instance = nullptr;
std::mutex Logger::mutex;



/**
 * Main function to set up and run a TCP echo server.
 */
int main(int argc, char* argv[]) {
    // Parse command-line arguments
    if (argc==1){
            std::cerr << "Usage: " << argv[0] << " -f <config_file_path>" << std::endl;
            return 1;
    }
    std::string configFilePath="";
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) { // Make sure we do not go out of bounds
            configFilePath = argv[++i]; // Increment 'i' to skip the file path in the next loop iteration
        } else {
            std::cerr << "Usage: " << argv[0] << " -f <config_file_path>" << std::endl;
            return 1;
        }
    }

    //read config ini file
    auto [readConfigResult,configFile]=readIniFile(configFilePath);
    if (readConfigResult>0){
            std::cerr << "Config file not found at  "<< configFilePath<<". Try with an absolute path." << std::endl;
        return 1;
    }

    //set appConfig
    Config appConfig;

    //add config params from settings.ini   
    appConfig.port=configFile.port;
    appConfig.baseURL=configFile.baseURL; 
    appConfig.logsDir=configFile.logsDir;
    appConfig.deviceSecurityParametersPath=configFile.deviceSecurityParametersPath;

    //create logger
    Logger* logger = Logger::getInstance();
    logger->init(appConfig);  // Initialize logger configuration once

    logger->log("POS SERVER"); 
    logger->log( "Port                        : " +std::to_string( configFile.port));
    logger->log( "Base URL                    : " + configFile.baseURL);
    logger->log( "deviceSecurityParametersPath: " + configFile.deviceSecurityParametersPath);
    logger->log( "logsDir                     : " + configFile.logsDir);


    //setup app: check secret tokens, activation, access token, etc 
    auto [setupResult,accessToken]=setup(appConfig);
    if (setupResult>0){
        logger->log("Exiting program as setup failed");
        std::cout<<"Exiting program..."<<std::endl;
        return 1;
    }else{
        startServer(accessToken,appConfig);
        return 0;
    }
}