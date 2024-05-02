#include <iostream>
#include <string>

#include "json.hpp"
#include <curl/curl.h>

using json = nlohmann::json;

// This function will be called by libcurl as it receives data
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *s) {
    size_t newLength = size * nmemb;
    try {
        s->append((char*)contents, newLength);
    } catch(std::bad_alloc &e) {
        // Handle memory couldn't be allocated error
        return 0;
    }
    return newLength;
}

int main() {
    CURL *curl;
    CURLcode res;
    std::string readBuffer; 
    curl = curl_easy_init();
    if(curl) {
        // Set the URL that receives the POST data
        curl_easy_setopt(curl, CURLOPT_URL, "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/device/activateDevice?Secret=SECRET");

        // Specify the POST data
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Perform the request, and get the response code
        res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }else{
            std::cout<<"OK"<<std::endl;
              // Try to parse the response as JSON
            try {
                json j = json::parse(readBuffer);
                std::cout << "JSON response: " << j.dump(4) << std::endl; // Pretty print the JSON
            } catch(json::parse_error &e) {
                std::cerr << "JSON parsing error: " << e.what() << '\n';
            }
        }

        // Always cleanup
        curl_easy_cleanup(curl);
    }
    return 0;
}