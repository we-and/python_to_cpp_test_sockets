#ifndef ACCESSTOKEN_H
#define ACCESSTOKEN_H
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
#include "../utils/log_utils.hpp"
#include "../utils/io_utils.hpp"
#include "config.hpp"

#include "../responses/activation_response.hpp"
#include "../responses/session_response.hpp"
namespace fs = std::filesystem;

std::string timePointToString(const std::chrono::system_clock::time_point& time_point) {
    // Convert time_point to time_t for easier formatting
    std::time_t time = std::chrono::system_clock::to_time_t(time_point);

    // Convert time_t to tm as local time
    std::tm tm = *std::localtime(&time);

    // Use stringstream to format the date and time
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // Return the formatted string
    return ss.str();
}

/**
 * Validates an access token based on its expiration time.
 *
 * This function checks the environment for an access token and its associated expiration time.
 * If both are present, it parses the expiration time and compares it against the current system time
 * to determine if the token is still valid. The expiration time is expected to be in the format "YYYY-MM-DD HH:MM:SS".
 * If the parsing fails or the token has expired, the function returns false.
 *
 * @return A bool indicating whether the access token is valid (true) or not (false).
 *
 * Usage Example:
 * bool valid = is_valid_access_token();
 *
 * Notes:
 * - This function assumes that the access token and its expiration time are stored in the environment variables
 *   'ACCESS_TOKEN' and 'TOKEN_EXPIRY_TIME', respectively.
 * - Proper error handling is implemented for date parsing and time comparison.
 */
bool is_valid_access_token()
{
    Logger *logger = Logger::getInstance();
    logger->log("is_valid_access_token");

    const char *access_token = std::getenv("ACCESS_TOKEN");
    const char *expiration_time_str = std::getenv("TOKEN_EXPIRY_TIME");

    if (access_token != nullptr && expiration_time_str != nullptr)
    {
     logger->log("is_valid_access_token has both");
     logger->log(access_token);
     logger->log(expiration_time_str);
        std::tm tm = {};
        std::istringstream ss(expiration_time_str);

        // This parses the date string in the specific format into the tm structure
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail())
        {
            std::cerr << "Failed to parse time string" << std::endl;
  logger->log("Token expiry failed");
            return false;
        }
  logger->log("Token expiry parsed");
        // Convert std::tm to std::chrono::system_clock::time_point
        auto expiration_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto current_time = std::chrono::system_clock::now();

        std::string exp_time_str = timePointToString(expiration_time);
        std::string cur_time_str = timePointToString(current_time);
            logger->log("Current time "+(cur_time_str));
            logger->log("Expiration time"+(exp_time_str));
        
        // Compare current time with expiration time
        if (current_time < expiration_time)
        {
            logger->log("Time ok");
            return true;
        }
        else
        {
            logger->log("Time expired");
            return false;
        }
    }else{
                    logger->log("No env");

    }
    return false;
}

/**
 * Calculates a SHA-256 hash of a device key concatenated with the incremented sequence value.
 *
 * This function logs the process of hashing including the initial data, the data ready for hashing,
 * and the final hash result. It takes a current sequence value and a device key, increments the
 * sequence value, concatenates it with the device key, and then computes the SHA-256 hash of the
 * resulting string. The hash is returned as a hexadecimal string.
 *
 * @param currentSequenceValue A std::string representing the current sequence number. It must be
 *                             convertible to an integer.
 * @param deviceKey A std::string representing the device key to be concatenated with the sequence value.
 *
 * @return A std::string containing the hexadecimal representation of the SHA-256 hash of the concatenated
 *         string of the device key and incremented sequence value.
 *
 * Usage Example:
 * std::string hash = calculateHash("1", "device123Key");
 *
 * Notes:
 * - This function uses the Crypto++ library to perform SHA-256 hashing.
 * - Exception handling should be added to manage potential std::stoi conversion failures.
 * - Ensure that proper logging mechanisms are set up to handle the output from `log`.
 */
std::string calculateHash(int deviceSequence, const std::string &deviceKey)
{
    Logger *logger = Logger::getInstance();
    logger->log("Calculating hash... Current Sequence Value: " + std::to_string(deviceSequence) + ", Device Key: " + deviceKey);

    // Convert string to integer and increment

    int nextSequenceValue = deviceSequence + 1;

    // Concatenate device key with next sequence value
    std::stringstream ss;
    ss << deviceKey << nextSequenceValue;
    std::string data_to_hash = ss.str();

    logger->log("Data to hash: " + data_to_hash);

    // SHA-256 Hash calculation
    CryptoPP::SHA256 hash;
    std::string digest;

    CryptoPP::StringSource(data_to_hash, true,
                           new CryptoPP::HashFilter(hash,
                                                    new CryptoPP::HexEncoder(
                                                        new CryptoPP::StringSink(digest))));

    //make lowercase
    std::transform(digest.begin(), digest.end(), digest.begin(),
                   [](unsigned char c) -> unsigned char { return std::tolower(c); });
    logger->log("Sequence Hash: " + digest);
    
    return digest;
}

bool isValidSecretToken(std::string token)
{
    return !token.empty();
}
int saveSecretToken(std::string secret, std::string posDirectory, std::string secretTokenFilename)
{
    Logger *logger = Logger::getInstance();
    logger->log("saveSecretToken");
    // Define the file path
    std::string filePath = posDirectory + secretTokenFilename;

    // Open a file in write mode
    std::ofstream outFile(filePath);

    if (!outFile)
    {
        std::cerr << "Error: Unable to open file at " << filePath << std::endl;
        return 1;
    }

    // Write the POS token to the file
    outFile << secret << std::endl;

    // Close the file
    outFile.close();
    logger->log("saveSecretToken chmod");
    // Set file permissions to read and write for owner only
    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);
    return 0;
}

// return isSuccess,hasValidSecretToken
std::pair<bool, bool> hasValidSecretToken(std::string posDirectory, std::string secretTokenFilename)
{
    Logger *logger = Logger::getInstance();
    logger->log("hasValidSecretToken");
    auto [checkSuccess, secretTokenExists] = checkFileExists(posDirectory, secretTokenFilename, logger);
    if (!checkSuccess)
    {
        return {false, false};
    }
    std::string secretToken;
    if (secretTokenExists)
    {
        fs::path secretTokenPath = posDirectory + secretTokenFilename;
        secretToken = readStringFromFile(secretTokenPath, logger);
        logger->log("    hasValidSecretToken true");
        return {true, isValidSecretToken(secretToken)};
    }
    else
    {

        /*
        bool allowUserToEnterToken=false;
        if(allowUserToEnterToken){
            secretToken=inputSecretToken();
            saveSecretToken(secretToken,posDirectory,secretTokenFilename);
            logger->log( "hasValidSecretToken false");
            return isValidSecretToken(secretToken);
        }else{
            */
        std::cout << "Secret token does not exist. Please run set_token <YOUR_SECRET_TOKEN> beforehand with admin rights." << std::endl;
        logger->log("    hasValidSecretToken false");

        return {true, false};
        // }
    }
}
enum SessionTokenCheck {
    SESSIONTOKENCHECK_NOT_FOUND,
    SESSIONTOKENCHECK_FOUND_EXPIRED,
    SESSIONTOKENCHECK_FOUND_VALID
};

// used at the request phase
SessionTokenCheck hasValidSessionToken(int requestorSocket)
{
    Logger *logger = Logger::getInstance();
    logger->log("hasValidSessionToken");
    bool sessionTokenExists = checkEnvVarExists("ACCESS_TOKEN", logger);
    if (sessionTokenExists)
    {
        logger->log("    hasValidSessionToken: has ACCESS_TOKEN");
        bool isValid= is_valid_access_token();
        if (isValid){
            return SESSIONTOKENCHECK_FOUND_VALID;
        }else{
            return SESSIONTOKENCHECK_FOUND_EXPIRED;
        }
    }
    else
    {
        logger->log("    hasValidSessionToken: no ACCESS_TOKEN");
        return SESSIONTOKENCHECK_NOT_FOUND;
    }
}

// used for the setup phase
SessionTokenCheck hasValidSessionTokenInit()
{
    Logger *logger = Logger::getInstance();
    logger->log("hasValidSessionTokenInit");
    bool sessionTokenExists = checkEnvVarExists("ACCESS_TOKEN", logger);
    if (sessionTokenExists)
    {
        logger->log("    hasValidSessionTokenInit: has ACCESS_TOKEN");
        bool isValid= is_valid_access_token(); //-1 means no resend to sender if token is invalid.
        if (isValid){
                return SESSIONTOKENCHECK_FOUND_VALID;
            }else{
                return SESSIONTOKENCHECK_FOUND_EXPIRED;
            }
    }
    else
    {
        logger->log("    hasValidSessionTokenInit: no ACCESS_TOKEN");
        return SESSIONTOKENCHECK_NOT_FOUND;
    }
}
std::string getFutureTime(int seconds) {
    // Get the current time
    auto now = std::chrono::system_clock::now();

    // Add the specified number of seconds to the current time
    std::chrono::seconds duration(seconds);
    auto future_time = now + duration;

    // Convert system_clock::time_point to time_t
    std::time_t future_time_t = std::chrono::system_clock::to_time_t(future_time);

    // Convert time_t to tm as local time
    std::tm tm = *std::localtime(&future_time_t);

    // Format the time using stringstream
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    // Return the formatted string
    return ss.str();
}
bool storeEnvironmentVariables(const std::string& filePath, const std::string& accessToken, const std::string& expiryTime) {
    Logger *logger = Logger::getInstance();
    logger->log("storing env in env file"+filePath);
    // Try to create the directory first (requires #include <sys/stat.h> and #include <sys/types.h>)
    // This step is optional based on whether directory exists or needs to be created by this function.
//    system(("mkdir -p " + filePath.substr(0, filePath.find_last_of('/'))).c_str());

    // Open or create the file to write the environment variables
    std::ofstream outputFile(filePath, std::ios::out | std::ios::trunc);  // Open in write mode and truncate existing
    if (!outputFile.is_open()) {
        std::cerr << "Failed to open or create the file: " << filePath << std::endl;
         logger->log("storing env failed to open");
        return false;
    }

    // Write the environment variables to the file
    outputFile << "ACCESS_TOKEN=" << accessToken << std::endl;
    outputFile << "TOKEN_EXPIRY_TIME=" << expiryTime << std::endl;

    if (outputFile.fail()) {
        std::cerr << "Failed to write to the file: " << filePath << std::endl;
         logger->log("storing env failed");
        outputFile.close();

        return false;
    }

    // Close the file
    outputFile.close();
     logger->log("stored env in env file"+filePath);
    return true;
}

std::pair<int, std::string> processActivateResponseErrorMessage(ActivateDeviceAPIResponse &response, Logger *logger)
{
    if (response.getMessage() == "Invalid Credentials")
    {
        std::cerr << "processActivateResponseErrorMessage:  exit with " << response.getMessage() << std::endl;
        logger->log("Exiting after 'Invalid Credentials'");
        std::exit(EXIT_FAILURE);
    }
    else if (response.getMessage() == "Invalid secret token")
    {
        std::cerr << "processActivateResponseErrorMessage: exit with " << response.getMessage() << std::endl;
        logger->log("Exiting after 'Invalid secret token'");
        std::exit(EXIT_FAILURE);
    }
    else
    {
        std::cerr << "Message " << response.getMessage() << std::endl;
    }
    return {1, ""}; // Return an error code
}

std::pair<int, std::string> processSessionResponseErrorMessage(SessionAPIResponse &response, Logger *logger){
     if (response.getMessage() == "Invalid Credential")
    {
        std::cerr << "processSessionResponseErrorMessage:  exit with " << response.getMessage() << std::endl;
        logger->log("Exiting after session request message Invalid Credential:" + response.getMessage());
        std::exit(EXIT_FAILURE);
        return {1, ""}; // Return an error code
    }else{
        std::cerr << "processSessionResponseErrorMessage:  exit with " << response.getMessage() << std::endl;
        logger->log("Exiting after session request message " + response.getMessage());
        std::exit(EXIT_FAILURE);
        return {1, ""}; // Return an error code

    }
}

// returns { 0 , accesstoken } if success
//         { 1, ""}            if not
std::pair<int, std::string> processActivateResponseOK(ActivateDeviceAPIResponse &activateResponse, Logger *logger, const Config &appConfig){
    logger->log("processActivateResponseOK");
    // Extract deviceId, deviceKey, and deviceSequence from the activation result
    int deviceId = activateResponse.getDeviceId();
    std::string deviceKey = activateResponse.getDeviceKey();
    int deviceSequence = activateResponse.getDeviceSequence();
    logger->log("processActivateResponseOK 1");

    // Calculate a hash using the device sequence and device key
    auto sequenceHash = calculateHash(deviceSequence, deviceKey);

    // Send the calculated sequence hash along with device details to create a session
    // Receive session creation result as a JSON object
    SessionAPIResponse response = SessionAPIResponse();
    bool isValid = response.session(deviceId, deviceSequence, deviceKey, sequenceHash, appConfig);
    std::cout << "sessionResult" << response.getRawJson() << std::endl;
    logger->log("Session has response");
    if (response.hasMessage())
    {
        logger->log("Session has response message");
        return processSessionResponseErrorMessage(response, logger);
    }

    logger->log("Response > accessToken="+response.getAccessToken() + " expiresTime="+std::to_string(response.getExpiryTime()));
    logger->log("Response > valid="+(isValid));
    if (response.hasAccessToken()){
        logger->log("Session has response with access token");

        // Extract the access token and its expiry time from the session result
        std::string accessToken = response.getAccessToken();
        int expiryTime = response.getExpiryTime();
        
        std::string  expiryTimeStr= getFutureTime(expiryTime);
        logger->log("Session has response accessToken="+accessToken + " expiryTime="+(expiryTimeStr));

        // Save the session result (which includes the access token and expiry) to a JSON file
        // saveJsonToFile(createSessionResult, accessTokenFilename);
        // Set the environment variable
        
        if (setenv("ACCESS_TOKEN", accessToken.c_str(), 1) != 0)
        {
             logger->log("Session has response accessToken="+accessToken + " expiryTime="+(expiryTimeStr));
            logger->log("Failed to set env variable ACCESS_TOKEN");

            std::cerr << "Failed to set environment variable." << std::endl;
            return {1, ""}; // Return an error code
        }
        if (setenv("TOKEN_EXPIRY_TIME",expiryTimeStr.c_str(), 1) != 0)
        {
            logger->log("Failed to set env variable TOKEN_EXPIRY_TIME");

            std::cerr << "Failed to set environment variable." << std::endl;
            return {1, ""}; // Return an error code
        }
        // Assuming function should return a bool, add return value here.
        // For example, you might return true if everything succeeds:

        storeEnvironmentVariables(appConfig.envFilePath, accessToken, expiryTimeStr);
        return {0, accessToken};
    }else{
        logger->log("Sesssion has response with neither message nor accesstoken");
    }
    logger->log("Session has response other");

    return {1, ""}; // Return an error code
}

// Function to process a one-time POS token for device activation and session creation
// returns { 0 , accesstoken } if success
//         { 1, ""}            if not
std::pair<int, std::string> requestAccessTokenFromSecretToken(std::string secretToken, const Config &config)
{
    Logger *logger = Logger::getInstance();
    logger->log("requestAccessTokenFromSecretToken");
    // Activate the device using the POS token and receive the activation result as a JSON object
    auto response = ActivateDeviceAPIResponse();
    bool isValid = response.activate(secretToken, config);
    //    json activationResult = activateDevice(secretToken,config);
    logger->log("requestAccessTokenFromSecretToken: has activationResult");

    if (response.hasMessage()){
        return processActivateResponseErrorMessage(response, logger);
    }
    if (response.hasDeviceId())
    {
        // Save the activation result to a JSON file
        saveJsonToFile(response.getRawJson(), config.deviceSecurityParametersPath);

        return processActivateResponseOK(response, logger, config);
    }
    else
    {
        return {1, ""}; // Return an error code
    }
}

#endif