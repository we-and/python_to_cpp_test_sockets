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
        json deviceSecurity=readJsonFromFile(appConfig.deviceSecurityParametersPath,logger);

        //load into struct
        ActivateDeviceAPIResponse response=ActivateDeviceAPIResponse();
        response.setDeviceId(deviceSecurity["deviceId"]);
        response.setDeviceSequence(deviceSecurity["deviceSequence"]);
        response.setDeviceKey(deviceSecurity["deviceKey"]);

        //increment device sequence 
        response.incrementDeviceSequence();

        //send as a REST /session sequest
        auto [sessionResult,accessToken]=processActivateResponseOK(response, logger, appConfig);
        if (sessionResult==0){
            
            logger->log("setup processActivateResponseOK success");
            


            //update device security parameters
            saveJsonToFile(response.getRawJson(), appConfig.deviceSecurityParametersPath);
            return {0,accessToken};
        }else{
            logger->log("sessionSuccess false");
            logger->log("setup processActivateResponseOK failed");
            return {1,""};
        }
        }

#endif