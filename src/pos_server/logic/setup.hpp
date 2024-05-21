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
    bool hasValidSessionToken_=hasValidSessionTokenInit();
    if (hasValidSessionToken){
        logger->log( "Setup hasValidSessionToken_ yes"  );  
        //if already setup
        const char* access_token = std::getenv("ACCESS_TOKEN");
        logger->log( "Setup hasValidSessionToken_ yes read"  );  
        logger->log( "Setup hasValidSessionToken_ yes read return"+std::string(access_token)  );  
        return {0,access_token};
    }else{

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

                //debug
                /*
                //send sequence hash directly
                                std::cout << "Debug send hash directly" <<std::endl;

                ActivateDeviceAPIResponse response=ActivateDeviceAPIResponse();
            std::string jsonstr = std::string("{    \"deviceId\": 8,    \"deviceKey\": \"2c624232cdd221771294dfbb310aca000a0df6ac8b66b696d90ef06fdefb64a3\", \"deviceSequence\": 1}");
            std::cout << "Debug parse" <<std::endl;
                logger->log("Force activation from"+jsonstr);
            bool isValid=response.parseAndValidateFromString(jsonstr);
                std::cout << "Debug"<<isValid <<std::endl;
                if (isValid){
                    auto [success,sessiontoken]=processActivateResponseOK(response,logger,appConfig);
                    std::cout << sessiontoken <<std::endl; 
                    return {success,sessiontoken};
                }else{
                    return {1,""};
                }
                //end debug
                */
                return requestAccessTokenFromSecretToken(secretToken,appConfig);
            }
        }else{
            logger->log( "Setup failed, no secret token"  );
            
            return {1,""};
        }
    }
}
#endif