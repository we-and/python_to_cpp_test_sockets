#ifndef SENDPT_H
#define SENDPT_H
#include <iostream>
#include "../utils/log_utils.hpp"
#include "../utils/json.hpp"
#include <string>
#include "../utils/request_utils.hpp"
using json = nlohmann::json;


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
int curl_debug_callback2(CURL *handle, curl_infotype type, char *data, size_t size, void *userptr) {
     Logger* logger = Logger::getInstance();
    if (type == CURLINFO_TEXT || type == CURLINFO_HEADER_IN || type == CURLINFO_HEADER_OUT) {
        std::string message(data, size); // Convert char* to string and trim null characters
        logger->log(message);
    }
    return 0; // Return 0 to indicate that everything is okay
}

std::string sendPlainText(const int requestorSocket, const std::string& accessToken, const std::string& payload, const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    std::string url = appConfig.baseURL + "posCommand";
       
    logger->log("Sending plain text... Access Token: " + accessToken + ", Payload: " + payload);
    logger->log("URL: " + url);

    CURL* curl = curl_easy_init();
    std::string response_string;
    std::string header_string;
    if (!curl) {
        return json();  // Early exit if curl initialization fails
    }
    logger->log("init: ");
     
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, ("Content-Type: text/plain"));
    headers = curl_slist_append(headers, ("Accept: text/plain"));
    headers = curl_slist_append(headers, ("Authorization: " + accessToken).c_str());
    logger->log("init: headers");
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, curl_debug_callback2);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, nullptr);

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    logger->log("parameters set");
    logger->log("payload"+payload);

    //error:logs stops here



    CURLcode res = curl_easy_perform(curl);
    logger->log("easy_perform");
    if (res == CURLE_OK) {
        logger->log("Request successful ");
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
        std::cerr << "Error parsing ISO8583 message: " <<  e.what() <<"\n"<<isoMessage<< std::endl;
logger->log("Error parsing ISO8583 message: " +isoMessage);
 logger->log( e.what() );
        return "";
    }

    
    return "";

}

std::string sendPlainTextLibevent(struct bufferevent *bev, const std::string& accessToken, const std::string& payload, const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    std::string url = appConfig.baseURL + "posCommand";
       
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
    curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0);

    CURLcode res = curl_easy_perform(curl);
    if (res == CURLE_OK) {
        logger->log("Request successful ");
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
            resendToRequestorLibevent(bev,payload);
            return response_string;
        }else{
            //If a "TOKEN EXPIRY" response is received the response code is to be modified to return
            //"DECLINED" in the ISO8583 message response located in field number 32, and “POS not
            //authorized to process this request” in field number 33.
            auto msg=modifyISO8583MessageForExpiredTokenAlert(payload);
            resendToRequestorLibevent(bev,msg);
            return "";
        }

    } catch (const std::exception& e) {
        std::cerr << "Error parsing ISO8583 message: " <<  e.what() <<"\n"<<isoMessage<< std::endl;
        logger->log("Error parsing ISO8583 message: " +isoMessage);
        logger->log( e.what() );
        return "";
    }

    
    return "";

}
#endif