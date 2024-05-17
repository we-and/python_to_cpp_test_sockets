#include <iostream>

#include "../json.hpp"
#include "config.hpp"
#include <string>
#include "session.hpp"
using json = nlohmann::json;

class SessionAPIResponse {
private:
    std::string accessToken;
    std::string expiresIn;

    std::string message;
    json raw;
public:
    // Constructor to initialize the members with default values
    SessionAPIResponse() : message("") {}

    // Function to parse JSON and validate the required fields
    bool parseAndValidate(const std::string& jsonString) {
        try {
            json j = json::parse(jsonString);


            if (j.contains("message")){
                if (!j["message"].is_string()){
                    return false;
                }
                message = j["message"];
            }else if  (j.contains("accessToken") && j["accessToken"].is_string()){
                // Check if all required fields are present and are of the correct type
                if (!j.contains("accessToken") || !j["accessToken"].is_string())
                    return false;
                if (!j.contains("expiresIn") || !j["expiresIn"].is_string())
                    return false;

                // Assign the values from JSON to the member variables
                accessToken = j["accessToken"];
                expiresIn = j["expiresIn"];

            }
            return true;
        } catch (const json::parse_error& e) {
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
    std::string getExpiresIn() const { return expiresIn; }
    std::string getMessage() const { return message; }

    json getRawJson() const { return raw;}

    bool parseFromJsonString(std::string jsonstr){
        bool isValid=parseAndValidate(jsonstr);
        return isValid;
    }

    bool session(const int deviceId, const int deviceSequence, const std::string& deviceKey, const std::string& sequenceHash, const Config& appConfig){
        auto jsonResponse=sendSequenceHash(deviceId, deviceSequence, deviceKey, sequenceHash,appConfig);
        raw=jsonResponse;
        bool isValid=parseAndValidate(jsonResponse);
        return isValid;
    }
};