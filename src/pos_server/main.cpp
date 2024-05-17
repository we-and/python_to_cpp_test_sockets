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

    #include <mutex>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cryptopp/sha.h>
    #include <cryptopp/hex.h>
    #include <cstdlib>
    #include <sys/stat.h> 

#include "configfile.hpp"
#include "log_utils.hpp"
#include "io_utils.hpp" 
#include "config.hpp"
#include "sendplaintext.hpp"
#include "logic/setup.hpp"
#include "logic/accesstoken.hpp"
#include "logic/server.hpp"
namespace fs = std::filesystem;


// Initialize static members
Logger* Logger::instance = nullptr;
std::mutex Logger::mutex;



// Retrieve the current system time as a time_t object
//auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

bool is_verbose=false;


/**
 * Logs a message to a uniquely named file with a timestamp.
 * 
 * This function takes a string message and logs it to a text file. The filename is uniquely generated based on the current system time.
 * Each log entry in the file is prefixed with a timestamp in the "YYYY-MM-DD HH:MM:SS" format, followed by the message itself.
 * 
 * @param text The message to be logged. It should be a string containing the text that needs to be recorded in the log file.
 * 
 * Usage Example:
 * logger->log("Application has started.");
 * 
 * Output File Example: log-1700819387.txt
 * Contents of log-1700819387.txt: 2024-04-30 12:34:56 - Application has started.
 */
using json = nlohmann::json;








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
    ConfigFile configFile=readIniFile(configFilePath);

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

    logger->log("main"); 
    logger->log( "Port: " +std::to_string( configFile.port));
    logger->log( "Base URL: " + configFile.baseURL);
    logger->log( "deviceSecurityParametersPath: " + configFile.deviceSecurityParametersPath);
    logger->log( "logsDir: " + configFile.logsDir);


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