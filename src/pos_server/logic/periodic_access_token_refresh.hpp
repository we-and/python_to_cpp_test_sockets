#ifndef PACCESSTOKEN_H
#define PACCESSTOKEN_H
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <stdlib.h>
#include <sstream>
#include <curl/curl.h>
#include <filesystem>
#include "../utils/json.hpp"

#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cstdlib>
#include <sys/stat.h>
#include "refreshtoken.hpp"
#include "../utils/log_utils.hpp"
#include "../utils/io_utils.hpp"
#include "config.hpp"

#include "../responses/activation_response.hpp"
#include "../responses/session_response.hpp"

std::pair<bool, std::tm *> get_expirytime_from_env()
{
    Logger *logger = Logger::getInstance();
   // logger->log("get_expirytime_from_env");
    const char *expiration_time_str = std::getenv("TOKEN_EXPIRY_TIME");

    std::tm tm = {};
    std::istringstream ss(expiration_time_str);

    // This parses the date string in the specific format into the tm structure
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail())
    {
        std::cerr << "Failed to parse time string" << std::endl;
        logger->log("TOKEN_EXPIRY_TIME parse failed");
        return {false, nullptr};
    }
//    logger->log("TOKEN_EXPIRY_TIME  parsed");
    // Convert std::tm to std::chrono::system_clock::time_point
    // auto expiration_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    return {true, &tm};
}
void askRefreshToken( const Config &appConfig)
{
    Logger *logger = Logger::getInstance();
    logger->log("askRefreshToken");

    // refresh token
    // Create a promise to hold the result of the setup function
    std::promise<std::pair<bool, std::string>> promise;
    std::future<std::pair<bool, std::string>> future = promise.get_future();

    // Run the setup function in a new thread
    std::thread requestnewtokenThread([&promise, &appConfig]()
                                      {
                try {
                    // Call the setup function and store the result in the promise
                    auto requestnewtokenResult = requestRefreshExpiredToken(appConfig);
                    promise.set_value(requestnewtokenResult);
                } catch (...) {
                    // In case of exception, set the exception in the promise
                    promise.set_exception(std::current_exception());
                } });

    // Wait for the setup function to complete
    requestnewtokenThread.join();

    auto [requestResult, newAccessToken] = future.get();
    if (requestResult == 0)
    {
        logger->log("askRefreshToken success");
    }
    else
    {
        logger->log("askRefreshToken failed");
    }
}
int checkIfOneMinuteBeforeExpiry( const Config &appConfig){
    Logger *logger = Logger::getInstance(); 
   // logger->log("checkIfOneMinuteBeforeExpiry");



        // Get the current time
        auto now = std::chrono::system_clock::now();
        std::time_t currentTime = std::chrono::system_clock::to_time_t(now);


        std::thread::id threadId = std::this_thread::get_id();
        // logger->log("Running in thread ID: " + std::to_string(std::hash<std::thread::id>{}(threadId)));


        auto [success, tm] = get_expirytime_from_env();
            if (success)
            {
                if (tm!=nullptr){
                // Convert targetDate to time_t
                std::time_t targetTime = std::mktime(const_cast<std::tm *>(tm));
                // Calculate the difference in seconds
                double difference = std::difftime(targetTime, currentTime);
                     logger->log("checkIfOneMinuteBeforeExpiry "+std::to_string(int(difference/60)-1)+"min before renewal");
    //           
                return int(difference/60)-1;
//                if (int(difference/60) % 5 )==0{
  //                  logger->log("checkIfOneMinuteBeforeExpiry "+std::to_string(int(difference/60))+"min before renewal");
    //            }
      //          // Check if the difference is 60 seconds (one minute)
        //        if (difference > 0 && difference <= 60)
          //      {
            //        askRefreshToken(appConfig);

                   
              //  }

                //}
            }
            }else{
                return -1;
            }
}

int checkTokenExpired( const Config &appConfig)
{
    //Logger *logger = Logger::getInstance();
  //  logger->log("checkTokenExpired");

      return   checkIfOneMinuteBeforeExpiry(appConfig);
}

void periodicTokenExpirationCheck( const Config &appConfig){
 Logger *logger = Logger::getInstance();
    logger->log("periodicTokenExpirationCheck");
   
    // Run the setup function in a new thread
    std::thread periodicCheckThread([ &appConfig]() {
           while (true) {
                int waittime_sec=                checkTokenExpired(appConfig);
                if (waittime_sec>-1){
                            // Sleep for the specified interval before checking again
                            std::this_thread::sleep_for(std::chrono::seconds(waittime_sec));
                            askRefreshToken(appConfig);
                }else{
                    
                }
            }

    });

    // Wait for the setup function to complete
    periodicCheckThread.detach();
}

#endif