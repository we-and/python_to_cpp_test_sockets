    #include <iostream>
    #include <fstream>
    #include <chrono>
    #include <string>
    #include <stdlib.h>  
    #include <sstream>
    #include <filesystem>
#include <future>

#include "configfile.hpp"
#include "utils/log_utils.hpp"
#include "utils/io_utils.hpp" 
#include "config.hpp"
#include "logic/setup.hpp"
#include "logic/accesstoken.hpp"
#include "logic/server.hpp"
#include "inifile.hpp"
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
    //search in pos folder if not a full abs path
    if (!isFullPath(configFilePath)){
        configFilePath="/home/ubuntu/pos/conf/"+configFilePath;
    }

    //read config ini file
    auto [readConfigResult,configFile]=readIniFile(configFilePath);
    if (readConfigResult>0){
            std::cerr << "Config file not found at  "<< configFilePath<<". Check within /home/ubuntu/pos/conf or try with an absolute path." << std::endl;
        return 1;
    }

    //set appConfig
    Config appConfig;

    //add config params from settings.ini   
    appConfig.port=configFile.port;
    appConfig.host=configFile.host;
    appConfig.envFilePath=configFile.envfilePath;
    appConfig.bufferSize=configFile.bufferSize;
    appConfig.serverDispatchMode=configFile.serverDispatchMode;
    appConfig.baseURL=configFile.baseURL; 
    appConfig.logsDir=configFile.logsDir;
    appConfig.deviceSecurityParametersPath=configFile.deviceSecurityParametersPath;

    //create logger
    Logger* logger = Logger::getInstance();
    logger->init(appConfig);  // Initialize logger configuration once

    logger->log("POS SERVER 0.94"); 
    logger->log( "Port                        : " +std::to_string( configFile.port));
    logger->log( "Base URL                    : " + configFile.baseURL);
    logger->log( "deviceSecurityParametersPath: " + configFile.deviceSecurityParametersPath);
    logger->log( "Host                        : " + configFile.host);
    logger->log( "logsDir                     : " + configFile.logsDir);
    logger->log( "appDir                      : " + configFile.appDir);
    logger->log( "serverExecutable            : " + configFile.serverExecutable);


    //setup app: check secret tokens, activation, access token, etc 

    // Create a promise to hold the result of the setup function
    std::promise<std::pair<bool, std::string>> promise;
    std::future<std::pair<bool, std::string>> future = promise.get_future();

    // Run the setup function in a new thread
    std::thread setupThread([&promise, &appConfig]() {
        try {
            // Call the setup function and store the result in the promise
            auto setupResult = setup(appConfig);
            promise.set_value(setupResult);
        } catch (...) {
            // In case of exception, set the exception in the promise
            promise.set_exception(std::current_exception());
        }
    });

    // Wait for the setup function to complete
    setupThread.join();

 // Get the result from the future
    try {
        auto [setupResult, accessToken] = future.get();
        //auto [setupResult,accessToken]=setup(appConfig);
            logger->log("Setup done");
        if (setupResult>0){
            logger->log("Exiting program as setup failed");
            std::cout<<"Exiting program..."<<std::endl;
            return 1;
        }else{

            startServer(accessToken,appConfig);
            return 0;
        }
      } catch (const std::exception& e) {
        std::cerr << "Setup encountered an exception: " << e.what() << std::endl;
    }

}