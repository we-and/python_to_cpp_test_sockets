#ifndef SESRES_H
#define SESRES_H
#include <iostream>

#include "../utils/json.hpp"
#include "config.hpp"
#include <string>
#include "session.hpp"
using json = nlohmann::json;

class SessionAPIResponse {
private:
    std::string accessToken;
    int expiryTime;

    std::string message;
    json rawJson;
public:
    // Constructor to initialize the members with default values
    SessionAPIResponse() : message("") {}

    // Function to parse JSON and validate the required fields
    bool parseAndValidateFromString(const std::string& jsonString) {
        try {
            json j = json::parse(jsonString);
            return parseAndValidate(j);

          } catch (const json::parse_error& e) {
            // Handle parsing errors (e.g., malformed JSON)
            std::cerr << "JSON parse error: " << e.what() << '\n';
            return false;
        }
    }
    // Function to parse JSON and validate the required fields
    bool parseAndValidate(const json& j) {
        Logger* logger = Logger::getInstance();
        try {
            logger->log("Parse session");
            if (j.contains("message")){
            logger->log("Parse session message");
                if (!j["message"].is_string()){
                    return false;
                }
                message = j["message"];
            }else if  (j.contains("accessToken") && j["accessToken"].is_string()){
                logger->log("Parse session accesstoken");

//                // Check if all required fields are present and are of the correct type
  //              if (!j.contains("accessToken") || !j["accessToken"].is_string())
    //                return false;
      //          if (!j.contains("expiryTime") || !j["expiryTime"].is_number_integer())
        //            return false;

                // Assign the values from JSON to the member variables
                accessToken = j["accessToken"];
                expiryTime = j["expiryTime"];
            }else{            
                logger->log("Parse session other");
}
            return true;
        } catch (const json::parse_error& e) {
                        logger->log("Parse session error");
                        logger->log(e.what());

            // Handle parsing errors (e.g., malformed JSON)
            std::cerr << "JSON parse error: " << e.what() << '\n';
            return false;
        }
    }
    bool hasAccessToken(){
        return accessToken!="";
    }
    bool hasMessage(){
        return message!="";
    }

    // Getters for each field
    std::string getAccessToken() const { return accessToken; }
    int getExpiryTime() const { return expiryTime; }
    std::string getMessage() const { return message; }

    json getRawJson() const { return rawJson;}


    bool session(const int deviceId, const int deviceSequence, const std::string& deviceKey, const std::string& sequenceHash, const Config& appConfig){
        Logger* logger = Logger::getInstance();
            logger->log("SessionAPIResponse session");

        json jsonResponse=sendSequenceHash(deviceId, deviceSequence, deviceKey, sequenceHash,appConfig);
            logger->log("SessionAPIResponse hash has response");

        rawJson=jsonResponse;
        bool isValid=parseAndValidate(jsonResponse);
        return isValid;
    }
};

#endif
