#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include "json.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>

using json = nlohmann::json;void log_to_file(const std::string& text) {
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ofstream log_file("log-" + std::to_string(t) + ".txt", std::ios::app);
    log_file << std::put_time(std::localtime(&t), "%F %T") << " - " << text << "\n";
}

json activateDevice(const std::string& secret) {
    CURL* curl = curl_easy_init();
    if (curl) {
        std::string url = "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/device/activateDevice?Secret=" + secret;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        // Add more curl options as needed

        char* output;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            json response = json::parse(output);
            curl_easy_cleanup(curl);
            return response;
        }
        curl_easy_cleanup(curl);
    }
    return json();
}

std::string calculateHash(const std::string& currentSequenceValue, const std::string& deviceKey) {
    log_to_file("Calculating hash... Current Sequence Value: " + currentSequenceValue + ", Device Key: " + deviceKey);

    // Convert string to integer and increment
    int currentValue = std::stoi(currentSequenceValue);
    int nextSequenceValue = currentValue + 1;

    // Concatenate device key with next sequence value
    std::stringstream ss;
    ss << deviceKey << nextSequenceValue;
    std::string data_to_hash = ss.str();

    log_to_file("Data to hash: " + data_to_hash);

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

    log_to_file("Sequence Hash: " + digest);
    return digest;
}

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* s) {
    size_t newLength = size * nmemb;
    size_t oldLength = s->size();
    try {
        s->resize(oldLength + newLength);
    } catch (std::bad_alloc& e) {
        // handle memory problem
        return 0;
    }

    std::copy((char*)contents, (char*)contents + newLength, s->begin() + oldLength);
    return size * nmemb;
}

json sendSequenceHash(const std::string& deviceId, const std::string& deviceSequence, const std::string& deviceKey, const std::string& sequenceHash) {
    std::string logMessage = "Sending sequence hash... Device ID: " + deviceId + ", Device Sequence: " + deviceSequence + ", Device Key: " + deviceKey + ", Sequence Hash: " + sequenceHash;
    log_to_file(logMessage);

    std::string url = "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/device/session?deviceId=" + deviceId + "&deviceSequence=" + deviceSequence + "&deviceKey=" + deviceKey + "&sequenceHash=" + sequenceHash;
    log_to_file("URL: " + url);

    json payload; // Assuming an empty JSON object is needed
    CURL* curl = curl_easy_init();
    std::string response_string;
    std::string header_string;

    if (curl) {
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

        log_to_file("Response: " + response_string);
        json response_json = json::parse(response_string);
        log_to_file("Response JSON: " + response_json.dump());

        return response_json;
    }

    return json(); // Return empty json object if curl initialization failed
}


void sendPlainText(const std::string& accessToken, const std::string& payload) {
    std::string url = "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/posCommand";
    log_to_file("Sending plain text... Access Token: " + accessToken + ", Payload: " + payload);
    log_to_file("URL: " + url);

    CURL* curl = curl_easy_init();
    std::string response_string;
    std::string header_string;

    if (curl) {
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
            log_to_file("Response: " + response_string);
            std::cout << "Request successful." << std::endl;
            std::cout << "Response from server: " << response_string << std::endl;
        } else {
            std::cout << "Request failed." << std::endl;
            std::cout << "Error: " << curl_easy_strerror(res) << std::endl;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

bool is_valid_access_token() {
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
        }
    }
    return false;
}

int main() {
    const char* host = "0.0.0.0";
    int port = 6000;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 6000
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Bind the socket to the host and port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server is listening on " << host << ":" << port << std::endl;

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        long valread = read(new_socket, buffer, 1024);
        std::string data(buffer, valread);
        std::cout << "Connection from " << inet_ntoa(address.sin_addr) << " established." << std::endl;
        log_to_file("Connection from " + std::string(inet_ntoa(address.sin_addr)) + " established.");
        log_to_file("Received data from client: " + data);

        if (!data.empty()) {
            send(new_socket, data.c_str(), data.size(), 0);
            log_to_file("Echoed back data to client.");
        }

        close(new_socket);
    }

    return 0;
}