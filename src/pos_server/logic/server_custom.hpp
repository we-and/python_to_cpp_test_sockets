#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <string>
#include <stdlib.h>  
#include <sstream>
#include <curl/curl.h>
#include <filesystem>
#include "sendplaintext.hpp"
#include <vector>
#include <thread>
#include <string>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>

#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cstdlib>
#include <sys/stat.h> 
#include "setup.hpp"


void handleClientCustom(int new_socket, struct sockaddr_in address, const std::string& sessionToken, const Config& appConfig, Logger* logger) {
    std::cout << "Connection from " << inet_ntoa(address.sin_addr) << " established." << std::endl;
    logger->log("Connection from " + std::string(inet_ntoa(address.sin_addr)) + " established.");  // Log connection

    int bufferSize=appConfig.bufferSize;
    std::vector<char> buffer(bufferSize); // Start with an initial size
    int totalBytesRead = 0;
    int bytesRead = 0;

    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(new_socket, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        int activity = select(new_socket + 1, &readfds, NULL, NULL, &timeout);
        if ((activity < 0) && (errno != EINTR)) {
            std::cerr << "Select error" << std::endl;
        }

        if (activity > 0 && FD_ISSET(new_socket, &readfds)) {
            bytesRead = read(new_socket, buffer.data() + totalBytesRead, buffer.size() - totalBytesRead);
            if (bytesRead > 0) {
                totalBytesRead += bytesRead;
                // Resize buffer if needed
                if (totalBytesRead == buffer.size()) {
                    buffer.resize(buffer.size() + bufferSize);
                }
            } else if (bytesRead == 0) {
                std::cout << "Client disconnected" << std::endl;
                break;
            } else {
                perror("read failed");
                break;
            }
        }

        std::string data(buffer.begin(), buffer.begin() + totalBytesRead);
        if (!data.empty()) {
            logger->log("Received data from client: " + data);  // Log received data

            // Process the data...
            // If data was received, echo it back to the client
            if (isISO8583(data)) {
                std::string payload = data;
                logger->log("valid isomsg");
                checkTokenAndExecute(new_socket, sessionToken, payload, appConfig);
            } else {
                logger->log("Not a valid isomsg");
                // Reject if not valid
                resendToRequestor(new_socket, data);
            }
            // Reset buffer for next read
            buffer.clear();
            buffer.resize(bufferSize);
            totalBytesRead = 0;
        }
    }

    close(new_socket);
}
void startServerCustom(std::string sessionToken,const Config& appConfig){
     Logger* logger = Logger::getInstance();
    logger->log("StartServer");
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = appConfig.port;  // Port number on which the server will listen for connections
   
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port 
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

   
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        std::thread(handleClientCustom, new_socket, address, sessionToken, std::ref(appConfig), logger).detach();
    }

    close(server_fd);
}