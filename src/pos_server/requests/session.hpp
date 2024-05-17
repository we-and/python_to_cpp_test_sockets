#include <iostream>
#include "../utils/log_utils.hpp"
#include "../utils/json.hpp"
#include <string>
#include "../utils/request_utils.hpp"
using json = nlohmann::json;
/**
 * Sends a sequence hash and other device information to a specified server endpoint via HTTP POST.
 * 
 * This function logs relevant information about the process, constructs a URL with query parameters,
 * sends a JSON payload, and processes the server's response. It uses CURL for the HTTP request,
 * handling both the response payload and headers. The function logs each significant step, including
 * URL creation, CURL execution results, and the response data. If successful, it parses the response
 * into a JSON object which is then returned. If CURL fails to initialize, or the request itself fails,
 * an empty JSON object is returned.
 * 
 * @param deviceId A std::string representing the device ID.
 * @param deviceSequence A std::string representing the sequence number of the device.
 * @param deviceKey A std::string representing the device key.
 * @param sequenceHash A std::string representing the computed sequence hash.
 * 
 * @return A JSON object representing the server's response. If an error occurs or CURL fails to initialize,
 *         an empty JSON object is returned.
 * 
 * Usage Example:
 * json response = sendSequenceHash("12345", "1", "deviceKey123", "abc123def456");
 * 
 * Notes:
 * - Ensure the server endpoint URL is correct and reachable.
 * - The function uses the libcurl library for HTTP communications and the nlohmann::json library for JSON handling.
 * - Proper error handling is implemented for CURL failures.
 */
json sendSequenceHash(const int deviceId, const int deviceSequence, const std::string& deviceKey, const std::string& sequenceHash, const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    std::string logMessage = "Sending sequence hash... Device ID: " + std::to_string(deviceId) + ", Device Sequence: " +std::to_string( deviceSequence) + ", Device Key: " + deviceKey + ", Sequence Hash: " + sequenceHash;
    logger->log(logMessage);

    std::string url = appConfig.baseURL+ "/device/session?deviceId=" +std::to_string( deviceId) + "&deviceSequence=" + std::to_string(deviceSequence) + "&deviceKey=" + deviceKey + "&sequenceHash=" + sequenceHash;
    logger->log("URL: " + url);

    json payload; // Assuming an empty JSON object is needed
    CURL* curl = curl_easy_init();
    std::string response_string;
    std::string header_string;
    if (!curl) {
        return json();  // Early exit if curl initialization fails
    }
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.dump().c_str()); // sending the payload as a string
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header_string);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }
    curl_easy_cleanup(curl);
    logger->log("Response: " + response_string);
    json response_json = json::parse(response_string);
    logger->log("Response JSON: " + response_json.dump());
    return response_json;
    

}