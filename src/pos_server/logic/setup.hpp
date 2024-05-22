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
#include "configfile.hpp"

#include "accesstoken.hpp"


//return {0,accesstoken} if success and has a token
//return {1,""} otherwise 
std::pair<int,std::string> setup(const Config& appConfig){
    Logger* logger = Logger::getInstance();
    auto posDirectory=appConfig.getPosDirectory();
    auto secretTokenFilename=appConfig.getSecretTokenFilename();
    
    logger->log( "Setup"  );     
    SessionTokenCheck hasValidSessionToken_=hasValidSessionTokenInit();
    if (hasValidSessionToken_==SESSIONTOKENCHECK_FOUND_VALID){
        logger->log( "Setup hasValidSessionToken_ yes"  );  
        //if already setup
        const char* access_token = std::getenv("ACCESS_TOKEN");
        logger->log( "Setup hasValidSessionToken_ yes read"  );  
        logger->log( std::string(access_token)  );  
        return {0,access_token};
    }else if (hasValidSessionToken_==SESSIONTOKENCHECK_FOUND_EXPIRED){
        
        //read device parameters 
        json deviceSecurity=readJsonFromFile(appConfig.deviceSecurityParametersPath);

        //load into struct
        ActivateDeviceAPIResponse response=ActivateDeviceAPIResponse();
        response.setDeviceId(deviceSecurity["deviceId"]);
        response.setDeviceSequence(deviceSecurity["deviceSequence"]);
        response.setDeviceKey(deviceSecurity["deviceKey"]);

        //increment device sequence 
        response.incrementDeviceSequence();

        //send as a REST /session sequest
        auto [sessionSuccess,sessionToken]=processActivateResponseOK(response, logger, config);
        if (sessionSuccess){
            //update device security parameters
            saveJsonToFile(response.getRawJson(), config.deviceSecurityParametersPath);
            return {0,access_token};
        }else{
            return {1,""};
        }


    }else{//if not found
        logger->log( "Setup hasValidSessionToken_ no"  );  
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

                return requestAccessTokenFromSecretToken(secretToken,appConfig);
            }
        }else{
            logger->log( "Setup failed, no secret token"  );
            
            return {1,""};
        }
    }
}
#endif