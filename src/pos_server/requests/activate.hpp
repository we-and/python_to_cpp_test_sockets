    #include <iostream>
    #include "../utils/log_utils.hpp"
    #include "../utils/json.hpp"
    #include <string>
    #include "../utils/request_utils.hpp"
    using json = nlohmann::json;
/**
 * Activates a device by making an HTTP POST request to a specific URL with a provided secret.
 * 
 * This function initializes a CURL session, constructs a request URL by appending a secret parameter,
 * and sends a POST request to the server to activate a device. If the request succeeds, it parses the
 * response into a JSON object which is then returned.
 * 
 * @param secret A std::string containing the secret key used to authorize the device activation.
 * 
 * @return A JSON object representing the server's response. If the CURL operation fails or if
 *         initialization fails, an empty JSON object is returned.
 * 
 * Usage Example:
 * json response = activateDevice("12345secret");
 * 
 * Expected JSON Response:
 * {
 *   "status": "success",
 *   "message": "Device activated"
 * }
 * 
 * Notes:
 * - Ensure the server endpoint is correct and reachable.
 * - This function depends on the libcurl library for HTTP communications and the nlohmann::json
 *   library for JSON parsing.
 */
json activateDevice(const std::string& secret, const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    logger->log("activateDevice");
    CURL *curl;
    CURLcode res;
    std::string readBuffer; 
    curl = curl_easy_init();
    if (!curl) {
        return json();  // Early exit if curl initialization fails
    }

        // Set the URL that receives the POST data
        std::string url = appConfig.baseURL + "device/activateDevice?Secret="+secret;
        logger->log("activateDevice: url="+url);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Specify the POST data
//        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request, and get the response code
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            logger->log("Failed");
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }else{
            logger->log("activateDevice: has response");
              // Try to parse the response as JSON
            try {
                json j = json::parse(readBuffer);
                logger->log( "activateDevice: JSON response start: "  ); // Pretty print the JSON
                logger->log(  j.dump(4) ); // Pretty print the JSON
                logger->log( "activateDevice: JSON response end." ); // Pretty print the JSON
                curl_easy_cleanup(curl);
                return j;
            } catch(json::parse_error &e) {
                std::cerr << "JSON parsing error: " << e.what() << '\n';
                logger->log( "activateDevice: JSON parsing error " );
                logger->log( e.what() );
            }
        }

        // Always cleanup
        curl_easy_cleanup(curl);
    return json();
}

