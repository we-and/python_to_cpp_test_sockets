#include <iostream>

#include "../utils/json.hpp"
#include <string>
#include "activate.hpp"
using json = nlohmann::json;

class ActivateDeviceAPIResponse {
private:
    int deviceId;
    std::string deviceKey;
    int deviceSequence;

    std::string message;
    json rawJson;
public:
    // Constructor to initialize the members with default values
    ActivateDeviceAPIResponse() : deviceId(-1), deviceSequence(-1),message("") {}

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
        logger->log("ActivateDeviceAPIResponse parseAndValidate");

        try {

            logger->log("    ActivateDeviceAPIResponse parseAndValidate parsed");
            if (j.contains("message")){
            logger->log("    ActivateDeviceAPIResponse parseAndValidate parsed message");
            
                if (!j["message"].is_string()){
                    return false;
                }
                message = j["message"];
            }else if  (j.contains("deviceId") && j["deviceId"].is_number_integer()){
                logger->log("    ActivateDeviceAPIResponse parseAndValidate parsed device");
            
                // Check if all required fields are present and are of the correct type
                if (!j.contains("deviceId") || !j["deviceId"].is_number_integer())
                    return false;
                if (!j.contains("deviceKey") || !j["deviceKey"].is_string())
                    return false;
                if (!j.contains("deviceSequence") || !j["deviceSequence"].is_number_integer())
                    return false;

                logger->log("    ActivateDeviceAPIResponse parseAndValidate parsed assign");
            
                // Assign the values from JSON to the member variables
                deviceId = j["deviceId"];
                deviceKey = j["deviceKey"];
                deviceSequence = j["deviceSequence"];

            }
            logger->log("    ActivateDeviceAPIResponse parseAndValidate parsed returns");
            return true;
        } catch (const json::parse_error& e) {
            // Handle parsing errors (e.g., malformed JSON)
            std::cerr << "JSON parse error: " << e.what() << '\n';
            logger->log("    ActivateDeviceAPIResponse parseAndValidate error");
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
    json getRawJson() const { return rawJson;}

    std::string getRawJsonString(){
       
        std::ostringstream jsonStream;
        jsonStream << "{\n";
        jsonStream << "    \"deviceId\": " << deviceId << ",\n";
        jsonStream << "    \"deviceKey\": \"" << deviceKey << "\",\n";
        jsonStream << "    \"deviceSequence\": " << deviceSequence << "\n";
        jsonStream << "}";
        return jsonStream.str();
    }
    /*
    void setRawJson() const {

    json j;
    j["deviceId"] = deviceId;
    j["deviceKey"] = deviceKey;
    j["deviceSequence"] = deviceSequence;

    rawJson= j; 
    Logger* logger = Logger::getInstance();
    logger->log("rawJson set as"+j.dump(4));

    }*/


     // Setter for deviceId
    void setDeviceId(int id) {
        deviceId = id;
    }

    // Setter for deviceKey
    void setDeviceKey(const std::string& key) {
        deviceKey = key;
    }

    // Setter for deviceSequence
    void setDeviceSequence(int sequence) {
        deviceSequence = sequence;
    }
    void incrementDeviceSequence(){
        deviceSequence=deviceSequence+1;
    }

    bool parseFromJsonString(std::string jsonstr){
         Logger* logger = Logger::getInstance();
         logger->log("ActivateDeviceAPIResponse parseFromJsonString:"+jsonstr);

        bool isValid=parseAndValidate(jsonstr);
        logger->log("ActivateDeviceAPIResponse parseFromJsonString parsed");

        return isValid;
    }

    bool activate(const std::string& secret, const Config& appConfig){
        json jsonResponse=activateDevice(secret,appConfig);
        rawJson=jsonResponse;
        bool isValid=parseAndValidate(jsonResponse);
        return isValid;
    }
};