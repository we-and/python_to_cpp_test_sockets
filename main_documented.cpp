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

/**
 * Logs a message to a uniquely named file with a timestamp.
 * 
 * This function takes a string message and logs it to a text file. The filename is uniquely generated based on the current system time.
 * Each log entry in the file is prefixed with a timestamp in the "YYYY-MM-DD HH:MM:SS" format, followed by the message itself.
 * 
 * @param text The message to be logged. It should be a string containing the text that needs to be recorded in the log file.
 * 
 * Usage Example:
 * log_to_file("Application has started.");
 * 
 * Output File Example: log-1700819387.txt
 * Contents of log-1700819387.txt: 2024-04-30 12:34:56 - Application has started.
 */
using json = nlohmann::json;
void log_to_file(const std::string& text) {
    // Retrieve the current system time as a time_t object
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Create or open a log file named with the current time stamp
    std::ofstream log_file("log-" + std::to_string(t) + ".txt", std::ios::app);

    // Write the current time and the log message to the file
    log_file << std::put_time(std::localtime(&t), "%F %T") << " - " << text << "\n";
}


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
 * - Ensure that proper logging mechanisms are set up to handle the output from `log_to_file`.
 */
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

/**
 * Callback function for handling data received from a CURL request.
 * 
 * This function is designed to be used with the libcurl easy interface as a write callback function.
 * It is triggered by CURL during data transfer operations, specifically when data is received from a server.
 * The function writes the received data into a provided std::string buffer. It ensures that the buffer is
 * resized to accommodate the incoming data and then copies the data into the buffer.
 * 
 * @param contents Pointer to the data received from the server. This data needs to be appended to the existing buffer.
 * @param size Size of each data element received in bytes.
 * @param nmemb Number of data elements received.
 * @param s Pointer to the std::string that will store the received data. This string is resized and filled with the data.
 * 
 * @return The total number of bytes processed in this call. If a memory allocation failure occurs (std::bad_alloc),
 *         the function returns 0, signaling to CURL that an error occurred.
 * 
 * Usage Example:
 * std::string response_data;
 * curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteCallback);
 * curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &response_data);
 * 
 * Notes:
 * - This function should be used carefully within a multithreaded context as it modifies the content of the passed std::string.
 * - Proper exception handling for std::bad_alloc is crucial to avoid crashes due to memory issues.
 */
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
 * - Proper initialization and cleanup of CURL and its resources are managed within the function.
 * - The function assumes that the server endpoint, headers, and CURL error handling are correctly set.
 */
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


/**
 * Main function to set up and run a TCP echo server.
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
int main() {
    const char* host = "0.0.0.0";  // Host IP address for the server (0.0.0.0 means all available interfaces)
    int port = 6001;  // Port number on which the server will listen for connections
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

        // Read data sent by the client
        long valread = read(new_socket, buffer, 1024);  // Read up to 1024 bytes from the client
        std::string data(buffer, valread);  // Convert read data to a C++ string
        std::cout << "Connection from " << inet_ntoa(address.sin_addr) << " established." << std::endl;
        log_to_file("Connection from " + std::string(inet_ntoa(address.sin_addr)) + " established.");  // Log connection
        log_to_file("Received data from client: " + data);  // Log received data

        // If data was received, echo it back to the client
        if (!data.empty()) {
            send(new_socket, data.c_str(), data.size(), 0);  // Send data back to client
            log_to_file("Echoed back data to client.");  // Log the action of echoing data
        }

        // Close the client socket after handling the connection
        close(new_socket);
    }

    // Although unreachable in an infinite loop, good practice is to return a code at the end
    return 0;
}