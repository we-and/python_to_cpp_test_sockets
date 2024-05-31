#ifndef REFRESH_H
#define REFRESH_H
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
#include "configfile.hpp"
#include "../utils/log_utils.hpp"
#include "accesstoken.hpp"

//return {0,accesstoken} if success and has a token
//return {1,""} otherwise 
std::pair<int,std::string> requestRefreshExpiredToken(const Config& appConfig){ 
 Logger* logger = Logger::getInstance();
    logger->log("requestRefreshExpiredToken SESSIONTOKENCHECK_FOUND_EXPIRED");
        //read device parameters 
        logger->log("requestRefreshExpiredToken read from "+        appConfig.deviceSecurityParametersPath);
        auto contents=readFileContents(appConfig.deviceSecurityParametersPath);
        logger->log( "requestRefreshExpiredToken File contents:" );
        logger->log( contents );

        logger->log( "requestRefreshExpiredToken readDeviceSecurityParameters readJsonFromFile:" );
        json deviceSecurity=readJsonFromFile(appConfig.deviceSecurityParametersPath,logger);
        logger->log( "requestRefreshExpiredToken readDeviceSecurityParameters readJsonFromFile done" );




        //load into struct
        logger->log( "requestRefreshExpiredToken load into response" );
        ActivateDeviceAPIResponse response=ActivateDeviceAPIResponse();
        response.setDeviceId(deviceSecurity["deviceId"]);
        response.setDeviceSequence(deviceSecurity["deviceSequence"]);
        response.setDeviceKey(deviceSecurity["deviceKey"]);
        //increment device sequence 
        logger->log( "requestRefreshExpiredToken load increment device sequence" );
        response.incrementDeviceSequence();

        logger->log( "requestRefreshExpiredToken send" );

        //send as a REST /session sequest
        auto [sessionResult,accessToken]=processActivateResponseOK(response, logger, appConfig);
        if (sessionResult==0){
            
            logger->log("requestRefreshExpiredToken processActivateResponseOK success");
            


            //update device security parameters
            saveJsonToFile(response.getRawJsonString(), appConfig.deviceSecurityParametersPath);
            return {0,accessToken};
        }else{
            logger->log("requestRefreshExpiredToken false");
            logger->log("requestRefreshExpiredToken processActivateResponseOK failed");
            return {1,""};
        }
        }

#endif