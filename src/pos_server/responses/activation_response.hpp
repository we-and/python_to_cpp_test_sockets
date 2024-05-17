#include <iostream>

#include "../json.hpp"
#include <string>
#include "activate.hpp"
using json = nlohmann::json;

class ActivateDeviceAPIResponse {
private:
    int deviceId;
    std::string deviceKey;
    int deviceSequence;

    std::string message;
    json raw;
public:
    // Constructor to initialize the members with default values
    ActivateDeviceAPIResponse() : deviceId(-1), deviceSequence(-1),message("") {}

    // Function to parse JSON and validate the required fields
    bool parseAndValidate(const std::string& jsonString) {
        Logger* logger = Logger::getInstance();
        logger->log("ActivateDeviceAPIResponse parseAndValidate");

        try {
            json j = json::parse(jsonString);

            logger->log("ActivateDeviceAPIResponse parseAndValidate parsed");
            if (!j.contains("message")){
            logger->log("ActivateDeviceAPIResponse parseAndValidate parsed message");
            
                if (!j["message"].is_string()){
                    return false;
                }
                message = j["message"];
            }else if  (j.contains("deviceId") && !j["deviceId"].is_number_integer()){
                logger->log("ActivateDeviceAPIResponse parseAndValidate parsed device");
            
                // Check if all required fields are present and are of the correct type
                if (!j.contains("deviceId") || !j["deviceId"].is_number_integer())
                    return false;
                if (!j.contains("deviceKey") || !j["deviceKey"].is_string())
                    return false;
                if (!j.contains("deviceSequence") || !j["deviceSequence"].is_number_integer())
                    return false;

logger->log("ActivateDeviceAPIResponse parseAndValidate parsed assign");
            
                // Assign the values from JSON to the member variables
                deviceId = j["deviceId"];
                deviceKey = j["deviceKey"];
                deviceSequence = j["deviceSequence"];

            }
            return true;
        } catch (const json::parse_error& e) {
            // Handle parsing errors (e.g., malformed JSON)
            std::cerr << "JSON parse error: " << e.what() << '\n';
logger->log("ActivateDeviceAPIResponse parseAndValidate error");
logger->log(e.what());

            return false;
        }
    }
    bool hasDeviceId(){
        return deviceId!=-1;
    }
    bool hasMessage(){
        return message!="";
    }

    // Getters for each field
    int getDeviceId() const { return deviceId; }
    std::string getDeviceKey() const { return deviceKey; }
    std::string getMessage() const { return message; }
    int getDeviceSequence() const { return deviceSequence; }
    json getRawJson() const { return raw;}

    bool parseFromJsonString(std::string jsonstr){
         Logger* logger = Logger::getInstance();
         logger->log("ActivateDeviceAPIResponse parseFromJsonString:"+jsonstr);
         json jsonResponse = json::parse(jsonstr);
        logger->log("ActivateDeviceAPIResponse parseFromJsonString parsed");

        bool isValid=parseAndValidate(jsonResponse);
        return isValid;
    }

    bool activate(const std::string& secret, const Config& appConfig){
        auto jsonResponse=activateDevice(secret,appConfig);
        raw=jsonResponse;
        bool isValid=parseAndValidate(jsonResponse);
        return isValid;
    }
};