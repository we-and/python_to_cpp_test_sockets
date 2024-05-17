#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <stdlib.h>  
#include <sstream>
#include <curl/curl.h>
#include <filesystem>
#include "json.hpp"

#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cstdlib>
#include <sys/stat.h> 
#include "log_utils.hpp"
#include "io_utils.hpp"
#include "config.hpp"
#include "responses/activation_response.hpp"
#include "responses/session_response.hpp"
namespace fs = std::filesystem;


// Initialize static members
Logger* Logger::instance = nullptr;
std::mutex Logger::mutex;


struct ConfigFile {
    int port;
    std::string baseURL;
    std::string logsDir;
    std::string deviceSecurityParametersPath;
};

// Retrieve the current system time as a time_t object
//auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

bool is_verbose=false;


/**
 * Logs a message to a uniquely named file with a timestamp.
 * 
 * This function takes a string message and logs it to a text file. The filename is uniquely generated based on the current system time.
 * Each log entry in the file is prefixed with a timestamp in the "YYYY-MM-DD HH:MM:SS" format, followed by the message itself.
 * 
 * @param text The message to be logged. It should be a string containing the text that needs to be recorded in the log file.
 * 
 * Usage Example:
 * logger->log("Application has started.");
 * 
 * Output File Example: log-1700819387.txt
 * Contents of log-1700819387.txt: 2024-04-30 12:34:56 - Application has started.
 */
using json = nlohmann::json;



//return the payload to the requestor
void resendToRequestor(int socket, std::string data){
    Logger* logger = Logger::getInstance();
    logger->log("resendToRequestor"+ data);
    send(socket,data.c_str(),data.size(), 0);  // Send data back to client

}

//Helper function to pad or trim a string to a specified fixed length
std::string formatFixedField(const std::string& value, size_t length) {
    if (value.length() > length) {
        return value.substr(0, length);  // Trim if too long
    } else {
        return value + std::string(length - value.length(), ' ');  // Pad with spaces if too short
    }
}

// Helper function to parse variable-length fields
std::string parseVariableField(const std::string& data, size_t& start, int maxLength) {
    int length = std::stoi(data.substr(start, 2));  // Assuming 2-digit length indicator
    start += 2;  // Move start past the length indicator

    if (length > maxLength) {
        throw std::runtime_error("Field length exceeds maximum allowed length.");
    }

    std::string field = data.substr(start, length);
    start += length;  // Update start position to the end of the current field
    return field;
}

// Function to modify fields 32 and 33 in an existing ISO 8583 message
std::string modifyISO8583MessageForExpiredTokenAlert(std::string isoMessage) {
    // Parse and modify the message
    std::string modifiedMessage;
    size_t index = 0;

    // Assuming fields are present and correctly formatted up to at least field 33
    // Parsing up to field 32
    modifiedMessage += isoMessage.substr(index, 85);  // Sum of lengths of fields before field 32
    index += 85;

    // Replace field 32 (length 12)
    modifiedMessage += formatFixedField("DECLINED", 12);
    index += 12;  // Skip the original content of field 32

    // Replace field 33 (length 40)
    modifiedMessage += formatFixedField("POS notauthorized to process this request", 40);
    index += 40;  // Skip the original content of field 33

    // Append the rest of the message unchanged
    modifiedMessage += isoMessage.substr(index);

    return modifiedMessage;
}



// Utility function to check if field 32 contains "TOKEN EXPIRY"
bool hasResponseTokenExpiry(const std::map<int, std::string>& fields) {
    auto it = fields.find(32);
    if (it != fields.end() && it->second == "TOKEN EXPIRY") {
        return true;
    }
    return false;
}



// Parses an ISO8583 response string into a map of fields.
std::map<int, std::string> parseISO8583(const std::string& response) {
    std::map<int, std::string> fields;
    size_t index = 0;

    // Fixed-length fields
    fields[0] = response.substr(index, 4);
    index += 4;

    // Variable-length fields
    fields[2] = parseVariableField(response, index, 20);  // Field 2 can be up to 20 characters long
    fields[4] = response.substr(index, 12);  // Fixed length
    index += 12;
    fields[10] = response.substr(index, 12);  // Fixed length
    index += 12;

    // More variable-length fields
    fields[11] = parseVariableField(response, index, 40);
    fields[12] = parseVariableField(response, index, 40);
    fields[13] = parseVariableField(response, index, 40);
    fields[14] = parseVariableField(response, index, 256);
    fields[15] = parseVariableField(response, index, 256);
    fields[16] = parseVariableField(response, index, 256);
    fields[22] = response.substr(index, 4);
    index += 4;

    fields[23] = parseVariableField(response, index, 12);
    fields[26] = parseVariableField(response, index, 20);
    fields[31] = parseVariableField(response, index, 40);
    fields[32] = response.substr(index, 12);
    index += 12;
    fields[33] = response.substr(index, 40);
    index += 40;

    // More fields including very large variable fields
    fields[35] = parseVariableField(response, index, 660);
    fields[36] = parseVariableField(response, index, 128);
    fields[49] = response.substr(index, 3);  // Assuming 3 characters for demo, adjust as needed
    index += 3;
    fields[50] = response.substr(index, 1);
    index += 1;

    fields[55] = parseVariableField(response, index, 20);
    fields[57] = parseVariableField(response, index, 128);
    fields[59] = parseVariableField(response, index, 20);
    fields[60] = response.substr(index, 1);
    index += 1;
    fields[61] = response.substr(index, 1);
    index += 1;
    fields[62] = response.substr(index, 1);
    index += 1;

    return fields;
}

// Function to prompt user and get a string
std::string getUserInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input); // Use getline to read the whole line
    return input;
}

std::string inputSecretToken(){
        std::cout << "Secret token does not exist. Please run set_token <YOUR_SECRET_TOKEN> beforehand or enter it below:" << std::endl;
        // Prompt for user input
        std::string userString = getUserInput("Please enter a secret token: ");
        return userString;
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
std::string calculateHash( int deviceSequence, const std::string& deviceKey) {
    Logger* logger = Logger::getInstance();
    logger->log("Calculating hash... Current Sequence Value: " +std::to_string( deviceSequence) + ", Device Key: " + deviceKey);

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
                new CryptoPP::StringSink(digest)
            )
        )
    );

    logger->log("Sequence Hash: " + digest);
    return digest;
}



/**
 * Sends a plain text payload to a specified server endpoint using HTTP POST with custom headers.
 * 
 * This function constructs an HTTP request to send a text-based payload to a server. It includes headers
 * for content type, accept type, and authorization using an access token. The function initializes a CURL
 * session, sets the necessary options for the URL, headers, and payload, and then performs the request.
 * The response from the server is logged and displayed. If the request is successful, it prints the server
 * response; otherwise, it reports an error. It logs the URL, the payload being sent, and the response received.
 * 
 * @param accessToken A std::string containing the access token for authorization.
 * @param payload A std::string containing the data to be sent to the server.
 * 
 * Usage Example:
 * sendPlainText("access_token123", "Hello, this is a test payload.");
 * 
 * Notes:
 * - This function is dependent on the libcurl library for handling HTTP communications.
 * - Assumes that the server endpoint, headers, and CURL error handling are correctly set.
 */
std::string sendPlainText(const int requestorSocket, const std::string& accessToken, const std::string& payload, const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    std::string url = appConfig.baseURL + "/posCommand";
       
    logger->log("Sending plain text... Access Token: " + accessToken + ", Payload: " + payload);
    logger->log("URL: " + url);

    CURL* curl = curl_easy_init();
    std::string response_string;
    std::string header_string;
    if (!curl) {
        return json();  // Early exit if curl initialization fails
    }
    
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("Content-Type: text/plain"));
    headers = curl_slist_append(headers, ("Accept: text/plain"));
    headers = curl_slist_append(headers, ("Authorization: " + accessToken).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        logger->log("Response: " + response_string);
        std::cout << "Request successful." << std::endl;
        std::cout << "Response from server: " << response_string << std::endl;
    } else {
        std::cout << "Request failed." << std::endl;
        std::cout << "Error: " << curl_easy_strerror(res) << std::endl;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);



    //parse and check for token expiry
    std::string isoMessage=response_string;
    try {
        auto parsedFields = parseISO8583(isoMessage);

        for (const auto& field : parsedFields) {
            std::cout << "Field " << field.first << ": " << field.second << std::endl;
        }
        //parse response for token expiry
        bool isTokenExpiry=hasResponseTokenExpiry(parsedFields);
        if (!isTokenExpiry){
            //If the ISO8583 response message does not contain a "TOKEN EXPIRY" response code
            //then the ISO8583 message response is to be returned to the requestor unmodified
            resendToRequestor(requestorSocket,payload);
            return response_string;
        }else{
            //If a "TOKEN EXPIRY" response is received the response code is to be modified to return
            //"DECLINED" in the ISO8583 message response located in field number 32, and “POS not
            //authorized to process this request” in field number 33.
            auto msg=modifyISO8583MessageForExpiredTokenAlert(payload);
            resendToRequestor(requestorSocket,msg);
            return "";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error parsing ISO8583 message: " << e.what() << std::endl;
        return "";
    }

    
    return "";

}
/*
// Generates an ISO 8583 response with specific values in fields 32 and 33
std::string generateDummyISO8583ResponseForExpiredTokenAlert() {
    std::ostringstream message;

    // Field 0: Message Type Indicator
    message << "0100";  // Assuming '0100' as a typical message type for a financial transaction request

    // Adding some dummy and specific data for other fields
    message << formatFixedField("123456789012", 12);  // Field 2: Primary Account Number (PAN), max length 20
    message << formatFixedField("000001234567", 12);  // Field 4: Transaction Amount

    // Skipping fields 10-11 for simplicity; add similar lines if needed
    message << formatFixedField("123", 3);  // Field 10: Local Transaction Time (Fixed length)
    message << formatFixedField("RESPONSE", 40);  // Field 11: Systems trace audit number (Variable length, up to 40)

    // Field 12: Local Transaction Time
    message << formatFixedField("123456", 6);  // Field 12: Local Transaction Time (Fixed length)
    message << formatFixedField("1206", 4);  // Field 13: Local Transaction Date

    // Assuming similar handling for fields 14-31 if necessary
    message << formatFixedField("FIELD14", 256);  // Field 14: Extended data (Variable length, up to 256)

    // Field 32: Acquiring Institution Identification Code
    message << formatFixedField("DECLINED", 12);  // Field 32 with fixed length of 12

    // Field 33: Forwarding Institution Identification Code
    message << formatFixedField("POS not authorized to process this request", 40);  // Field 33 with fixed length of 40

    // Fields 35-62: Dummy data; provide real values in actual implementation
    message << formatFixedField("ACCNO1234567890", 16);  // Field 35: Card Sequence Number (Variable length, up to 660)
    message << formatFixedField("DATA", 4);  // Field 36: Track 3 data (Variable length, up to 128)

    // Continuing with similar placeholders for the rest of the fields
    message << formatFixedField("", 0);  // Field 49: Currency Code, up to 128 characters
    message << formatFixedField("1", 1);  // Field 50: Service Restriction Code
    message << formatFixedField("055", 3);  // Field 55: ICC data (Variable, up to 20)

    message << formatFixedField("1", 1);  // Field 60: POS entry mode
    message << formatFixedField("1", 1);  // Field 61: POS condition code
    message << formatFixedField("1", 1);  // Field 62: POS capture code

    return message.str();
}
*/

/*
//Sends TOKEN EXPIRY as ISO8583 and manages its response
void executeTokenExpiry(int requestorSocket, std::string accessToken){
     std::string payload = R"(
        <isomsg>
      <!-- org.jpos.iso.packager.XMLPackager -->
      <field id="32" value="TOKEN EXPIRY"/>
    </isomsg>
    )";
    logger->log("Send to ")
    auto response=sendPlainText(requestorSocket, accessToken,payload);
    if (response=="TOKEN EXPIRY"){
        std::string payload = R"(
            <isomsg>
        <!-- org.jpos.iso.packager.XMLPackager -->
        <field id="32" value="DECLINED"/>
        <field id="33" value="“POS not authorized to process this request"/>
        </isomsg>
        )";
        auto response=sendPlainText(accessToken,payload);

    }else{
        resendToRequestor(requestorSocket,response);
    }
}*/

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
bool is_valid_access_token(std::optional<int> requestorSocket = std::nullopt) {
    Logger* logger = Logger::getInstance();
          logger->log( "is_valid_access_token" ); 

    const char* access_token = std::getenv("ACCESS_TOKEN");
    const char* expiration_time_str = std::getenv("TOKEN_EXPIRY_TIME");

    if (access_token != nullptr && expiration_time_str != nullptr) {
        std::tm tm = {};
        std::istringstream ss(expiration_time_str);

        // This parses the date string in the specific format into the tm structure
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        if (ss.fail()) {
            std::cerr << "Failed to parse time string" << std::endl;
            return false;
        }

        // Convert std::tm to std::chrono::system_clock::time_point
        auto expiration_time = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        auto current_time = std::chrono::system_clock::now();

        // Compare current time with expiration time
        if (current_time < expiration_time) {
            return true;
        }else{
            //optional:
            //the program checks token expiry at startup and when API returns TOKEN EXPIRY in field 32
            //if you want to check token expiration before the api, uncomment below
            //if expired, send TOKEN EXPIRY and manage its reponse
            //if (requestorSocket.has_value()) {
              //  executeTokenExpiry(*requestorSocket,access_token);
            //}
            return false;
        }
    }
    return false;
}

bool isValidSecretToken(std::string token){
    return !token.empty();
}
int saveSecretToken(std::string secret,std::string posDirectory,std::string secretTokenFilename){
    Logger* logger = Logger::getInstance();
    logger->log( "saveSecretToken"); 
        // Define the file path
    std::string filePath = posDirectory+secretTokenFilename;

    // Open a file in write mode
    std::ofstream outFile(filePath);

    if (!outFile) {
        std::cerr << "Error: Unable to open file at " << filePath << std::endl;
        return 1;
    }

    // Write the POS token to the file
    outFile << secret << std::endl;

    // Close the file
    outFile.close();
    logger->log( "saveSecretToken chmod"); 
 // Set file permissions to read and write for owner only
    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);
    return 0;
}

//return isSuccess,hasValidSecretToken
std::pair<bool,bool> hasValidSecretToken(std::string posDirectory,std::string secretTokenFilename){
    Logger* logger = Logger::getInstance();
    logger->log( "hasValidSecretToken"); 
    auto [checkSuccess,secretTokenExists]= checkFileExists(posDirectory,secretTokenFilename,logger);
    if (!checkSuccess) {return {false,false};}
    std::string secretToken;    
    if(secretTokenExists){
        fs::path secretTokenPath = posDirectory+secretTokenFilename;
        secretToken=readStringFromFile(secretTokenPath,logger);
        logger->log( "hasValidSecretToken true"); 
        return {true,isValidSecretToken(secretToken)};
    }else{
        
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
            logger->log("hasValidSecretToken false");
        
            return {true,false};
       // }
    }
}
//used at the request phase
bool hasValidSessionToken(int requestorSocket){
    Logger* logger = Logger::getInstance();
    logger->log("hasValidSessionToken");
    bool sessionTokenExists=checkEnvVarExists("ACCESS_TOKEN",logger);
    if(sessionTokenExists){
            logger->log("hasValidSessionToken: has ACCESS_TOKEN");
            return is_valid_access_token( requestorSocket);
    }else{
            logger->log("hasValidSessionToken: no ACCESS_TOKEN");
        return false;
    }
}
//used for the setup phase
bool hasValidSessionTokenInit(){
    Logger* logger = Logger::getInstance();
    logger->log("hasValidSessionToken");
    bool sessionTokenExists=checkEnvVarExists("ACCESS_TOKEN",logger);
    if(sessionTokenExists){
            logger->log("hasValidSessionToken: has ACCESS_TOKEN");
            return is_valid_access_token(); //-1 means no resend to sender if token is invalid. 
    }else{
            logger->log("hasValidSessionToken: no ACCESS_TOKEN");
        return false;
    }
}
void saveJsonToFile(const json& j, const std::string& filePath) {
   Logger* logger = Logger::getInstance();
    logger->log( "saveJsonToFile"  );
    std::ofstream file(filePath);
    if (!file) {
        std::cerr << "Error opening file for writing: " << filePath << std::endl;
        return;
    }
    file << j.dump(4); // Serialize the JSON with an indentation of 4 spaces
    file.close();
    if (file.good()) {
        logger->log( "JSON data successfully saved to " + filePath );
    } else {
        std::cerr << "Error occurred during file write operation." << std::endl;
    }
}


 
std::pair<int,std::string> processActivateResponseErrorMessage(ActivateDeviceAPIResponse &response,Logger * logger){
    if (response.getMessage()=="Invalid Credentials"){
                std::cerr <<"processActivateResponseErrorMessage:  exit with " << response.getMessage()<<std::endl;
                logger->log("Exiting after 'Invalid Credentials'");
                std::exit(EXIT_FAILURE);
         }else if (response.getMessage()=="Invalid secret token"){
                std::cerr <<"processActivateResponseErrorMessage: exit with "<< response.getMessage()<<std::endl;
                logger->log("Exiting after 'Invalid secret token'");
                std::exit(EXIT_FAILURE);
         }else{
                std::cerr <<"Message "<< response.getMessage()<<std::endl;
         }
         return {1,""}; // Return an error code
}

std::pair<int,std::string> processSessionResponseErrorMessage(SessionAPIResponse &response,Logger * logger){
                std::cerr <<"processSessionResponseErrorMessage:  exit with " << response.getMessage()<<std::endl;
                logger->log("Exiting after session request message "+response.getMessage());
                std::exit(EXIT_FAILURE);
         return {1,""}; // Return an error code
}

// returns { 0 , accesstoken } if success
//         { 1, ""}            if not
std::pair<int,std::string> processActivateResponseOK(ActivateDeviceAPIResponse &activateResponse,Logger * logger, const Config &config){
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
        SessionAPIResponse response=SessionAPIResponse();
        bool isValid=response.session(deviceId, deviceSequence, deviceKey, sequenceHash,config);
        std::cout <<"sessionResult"<< response.getRawJson()<<std::endl;

         if(response.hasMessage()){
         return processSessionResponseErrorMessage(response,logger);
    }
    if(response.hasAccessToken()){
        // Extract the access token and its expiry time from the session result
        std::string accessToken = response.getAccessToken();
        std::string expiresIn = response.getExpiresIn();

        // Save the session result (which includes the access token and expiry) to a JSON file
        //saveJsonToFile(createSessionResult, accessTokenFilename);
        // Set the environment variable
        if (setenv("ACCESS_TOKEN", accessToken.c_str(), 1) != 0) {
            std::cerr << "Failed to set environment variable." << std::endl;
            return {1,""}; // Return an error code
        }
        if (setenv("TOKEN_EXPIRY_TIME", expiresIn.c_str(), 1) != 0) {
            std::cerr << "Failed to set environment variable." << std::endl;
            return {1,""}; // Return an error code
        }
        // Assuming function should return a bool, add return value here.
        // For example, you might return true if everything succeeds:
        return {0,accessToken};
    }
           return {1,""}; // Return an error code
     
}


// Function to process a one-time POS token for device activation and session creation
// returns { 0 , accesstoken } if success
//         { 1, ""}            if not
std::pair<int,std::string> requestAccessTokenFromSecretToken(std::string secretToken, const Config &config ){
    Logger* logger = Logger::getInstance();
    logger->log( "requestAccessTokenFromSecretToken" ); 
    // Activate the device using the POS token and receive the activation result as a JSON object
    auto response=ActivateDeviceAPIResponse();
    bool isValid=response.activate(secretToken,config);
    //    json activationResult = activateDevice(secretToken,config);
    logger->log("requestAccessTokenFromSecretToken: has activationResult");

    if(response.hasMessage()){
         return processActivateResponseErrorMessage(response,logger);
    }
    if(response.hasDeviceId()){
        // Save the activation result to a JSON file
        saveJsonToFile(response.getRawJson(), config.deviceSecurityParametersPath);

        return processActivateResponseOK(response,logger,config);
        
    }else{
        return {1,""}; // Return an error code
    }
}
//return {0,accesstoken} if success and has a token
//return {1,""} otherwise 
std::pair<int,std::string> setup(const Config& appConfig){
    Logger* logger = Logger::getInstance();
    auto posDirectory=appConfig.getPosDirectory();
    auto secretTokenFilename=appConfig.getSecretTokenFilename();
    
    logger->log( "Setup"  );     
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
            //send sequence hash directly
                            std::cout << "Debug send hash directly" <<std::endl;

            ActivateDeviceAPIResponse response=ActivateDeviceAPIResponse();
          std::string jsonstr = std::string("{    \"deviceId\": 8,    \"deviceKey\": \"2c624232cdd221771294dfbb310aca000a0df6ac8b66b696d90ef06fdefb64a3\", \"deviceSequence\": 1}");
         std::cout << "Debug parse" <<std::endl;
            logger->log("Force activation from"+jsonstr);
           bool isValid=response.parseFromJsonString(jsonstr);
            std::cout << "Debug"<<isValid <<std::endl;
            if (isValid){
                auto [success,sessiontoken]=processActivateResponseOK(response,logger,appConfig);
                std::cout << sessiontoken <<std::endl; 
                return {success,sessiontoken};
            }else{
                 return {1,""};
            }
            //end debug
//            return requestAccessTokenFromSecretToken(secretToken,appConfig);
        }
    }else{
        logger->log( "Setup failed, no secret token"  );
        
        return {1,""};
    }
}



// The execute function orchestrates the communication with the API.
void checkTokenAndExecute(int requestorSocket, std::string sessionToken, std::string payload,const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    logger->log("execute");  // Log the action of echoing data

    //if the remote server endpoint determines the Session Token is not valid it will send an
    //ISO8583 response with the response code "TOKEN EXPIRY" in the ISO8583 message
    //response located in field number 32 prior to returning the ISO8583 message response to
    //the requestor
    auto isValidToken=is_valid_access_token( requestorSocket);
    if (!isValidToken){
        //redo setup for getting a new access token
        auto [isNewSetupValid,newAccessToken]=setup(appConfig);
        //if new setup is valid, process the request
        if (isNewSetupValid){
            sendPlainText(requestorSocket,newAccessToken, payload,appConfig);
        }else{
            //if failed again, return to user
            resendToRequestor( requestorSocket,payload);
        }
    }else{
    // Sends the XML payload using the session's access token.
        sendPlainText(requestorSocket,sessionToken, payload,appConfig);

    }
}




// Dynamic Buffer Resizing to handle reading for a TCP socket 
void handleClient(int new_socket, struct sockaddr_in address,std::string sessionToken,const Config& appConfig,Logger * logger) {
    std::cout << "Connection from " << inet_ntoa(address.sin_addr) << " established." << std::endl;
    logger->log("Connection from " + std::string(inet_ntoa(address.sin_addr)) + " established.");  // Log connection

    std::vector<char> buffer(1024); // Start with an initial size
    int totalBytesRead = 0;
    int bytesRead = 0;

    while ((bytesRead = read(new_socket, buffer.data() + totalBytesRead, buffer.size() - totalBytesRead)) > 0) {
        totalBytesRead += bytesRead;
        // Resize buffer if needed (you may also choose to increase in chunks, e.g., another 1024 bytes)
        if (totalBytesRead == buffer.size()) {
            buffer.resize(buffer.size() + 1024);
        }
    }

    if (bytesRead < 0) {
        perror("read failed");
        return;
    }

    std::string data(buffer.begin(), buffer.begin() + totalBytesRead);
    std::cout << "Received data: " << data << std::endl;

    logger->log("Received data from client: " + data);  // Log received data


    // Process the data...
    // If data was received, echo it back to the client
    if (!data.empty()) {
        std::string payload=data;
        checkTokenAndExecute(new_socket, sessionToken,payload,appConfig);
    }

}

/**
 * Runs a TCP echo server.
 * 
 * This function initializes a server socket, binds it to a specified host and port, listens for incoming connections,
 * and handles them by echoing back any received data. The server runs indefinitely until it encounters a failure in
 * socket operations like bind, listen, or accept.
 * 
 * Function Flow:
 * 1. Create a socket.
 * 2. Set socket options to reuse the address and port.
 * 3. Bind the socket to a host (IP address) and port.
 * 4. Listen on the socket for incoming connections.
 * 5. Accept a connection from a client.
 * 6. Read data from the client, log the received data, and send it back (echo).
 * 7. Close the connection and wait for another.
 * 
 * Error Handling:
 * - If any socket operation fails, the function will print an error message and terminate the program.
 * 
 * Logging:
 * - Logs the establishment of connections and the data received from clients.
 * 
 * Usage Example:
 * Run the compiled program, and it will start a server listening on IP 0.0.0.0 and port 6000.
 * 
 * Notes:
 * - The server handles one connection at a time.
 * - Ensure that the program is run with sufficient privileges to bind to the desired port.
 */

void startServer(std::string sessionToken,const Config& appConfig){
    Logger* logger = Logger::getInstance();
    const char* host = "0.0.0.0";  // Host IP address for the server (0.0.0.0 means all available interfaces)
    int port = appConfig.port;  // Port number on which the server will listen for connections
    int server_fd, new_socket;  // Socket file descriptors: one for the server, one for client connections
    struct sockaddr_in address;  // Structure to store the server's address information
    int opt = 1;  // Option value for setsockopt to enable certain socket properties
    int addrlen = sizeof(address);  // Length of the address data structure
    char buffer[1024] = {0};  // Buffer to store incoming data from clients

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");  // Print error message if socket creation fails
        exit(EXIT_FAILURE);  // Exit program with a failure return code
    }

    // Forcefully attach socket to the port 6001
    // This helps in reusing the port immediately after the server terminates.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");  // Print error message if setting socket options fails
        exit(EXIT_FAILURE);  // Exit program with a failure return code
    }

    // Setting the fields of the address structure
    address.sin_family = AF_INET;  // Address family (IPv4)
    address.sin_addr.s_addr = INADDR_ANY;  // IP address to bind to (all local interfaces)
    address.sin_port = htons(port);  // Convert port number from host byte order to network byte order

    // Bind the socket to the IP and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");  // Print error message if bind fails
        exit(EXIT_FAILURE);  // Exit program with a failure return code
    }

    // Listen for incoming connections with a backlog limit of 5 clients
    if (listen(server_fd, 5) < 0) {
        perror("listen");  // Print error message if listening fails
        exit(EXIT_FAILURE);  // Exit program with a failure return code
    }
    std::cout << "Server is listening on " << host << ":" << port << std::endl;  // Notify that server is ready

    // Infinite loop to continuously accept client connections
    while (true) {
        // Accept a new connection
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");  // Print error message if accepting a new connection fails
            exit(EXIT_FAILURE);  // Exit program with a failure return code
        }

         handleClient(new_socket, address,sessionToken,appConfig,logger);
        // Close the client socket after handling the connection
        close(new_socket);
    }

}


//GET PATH OF CURRENT EXECUTABLE, used to find settings.ini 
#if defined(_WIN32)
#include <windows.h>
std::string getExecutablePath() {
    char path[MAX_PATH] = { 0 };
    if (GetModuleFileNameA(NULL, path, MAX_PATH) > 0) {
        return std::filesystem::path(path).parent_path().string();
    }
    return std::string();
}
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
std::string getExecutablePath() {
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::filesystem::path(path).parent_path().string();
    }
    return std::string();
}
#elif defined(__linux__)
#include <unistd.h>
#include <linux/limits.h>
std::string getExecutablePath() {
    char result[PATH_MAX];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count > 0) {
        // Create a filesystem path from the result buffer and get the parent path
        std::filesystem::path exePath(result, result + count);
        return exePath.parent_path().string();
    }
    return std::string();  // Return an empty string if failed
}
#else
#error "Unsupported platform."
#endif
std::string getAbsolutePathRelativeToExecutable(const std::string& filename) {
    std::filesystem::path exePath = getExecutablePath();
    std::filesystem::path filePath = exePath / filename;
    try {
        // Resolve to absolute path and normalize
        return std::filesystem::absolute(filePath).string();
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return std::string();
    }
}

std::string getAbsolutePath(const std::string& filename) {
    std::filesystem::path path = filename;
    try {
        // Resolve to absolute path and normalize
        return std::filesystem::absolute(path).string();
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
        return std::string();
    }
}


ConfigFile readIniFile(const std::string& filename) {
   std::string absPath2= getAbsolutePathRelativeToExecutable(filename);
    std::cout <<"Reading config file from"<< absPath2 <<std::endl;
 //    std::string absolutePath = getAbsolutePath(filename);
   // std::cout << absolutePath <<std::endl;
    std::ifstream file(absPath2);
    ConfigFile config;
    std::string line;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return config; // Return default-initialized config if file opening fails
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        if (std::getline(iss, key, '=')) {
            std::string value;
            if (std::getline(iss, value)) {
                // Remove potential whitespace that might be around the '='
                key.erase(std::remove_if(key.begin(), key.end(), isspace), key.end());
                value.erase(std::remove_if(value.begin(), value.end(), isspace), value.end());

                if (key == "PORT") {
                    config.port = std::stoi(value);
                } else if (key == "BASE_URL") {
                    config.baseURL = value;
                }else if (key == "LOGS_DIR") {
                    config.logsDir = value;
                }else if (key == "DEVICE_SECURITY_PARAMETERS_PATH") {
                    config.deviceSecurityParametersPath = value;
                }
            }
        }
    }

    file.close();
    return config;
}


/**
 * Main function to set up and run a TCP echo server.
 */
int main(int argc, char* argv[]) {

    // Parse command-line arguments
    if (argc==1){
            std::cerr << "Usage: " << argv[0] << " -f <config_file_path>" << std::endl;
            return 1;
    }
    std::string configFilePath="";
    // Parse command-line arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) { // Make sure we do not go out of bounds
            configFilePath = argv[++i]; // Increment 'i' to skip the file path in the next loop iteration
        } else {
            std::cerr << "Usage: " << argv[0] << " -f <config_file_path>" << std::endl;
            return 1;
        }
    }


    //read config ini file
    ConfigFile configFile=readIniFile(configFilePath);

    //set appConfig
    Config appConfig;

    //add config params from settings.ini   
    appConfig.port=configFile.port;
    appConfig.baseURL=configFile.baseURL; 
    appConfig.logsDir=configFile.logsDir;
    appConfig.deviceSecurityParametersPath=configFile.deviceSecurityParametersPath;

    //create logger
    Logger* logger = Logger::getInstance();
    logger->init(appConfig);  // Initialize logger configuration once

    logger->log("main"); 
    logger->log( "Port: " +std::to_string( configFile.port));
    logger->log( "Base URL: " + configFile.baseURL);
    logger->log( "deviceSecurityParametersPath: " + configFile.deviceSecurityParametersPath);
    logger->log( "logsDir: " + configFile.logsDir);


    //setup app: check secret tokens, activation, access token, etc 
    auto [setupResult,accessToken]=setup(appConfig);
    if (setupResult>0){
        logger->log("Exiting program as setup failed");
        std::cout<<"Exiting program..."<<std::endl;
        return 1;
    }else{
        startServer(accessToken,appConfig);
        return 0;
    }
}