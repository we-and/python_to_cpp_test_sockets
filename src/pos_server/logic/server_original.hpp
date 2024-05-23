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

// Dynamic Buffer Resizing to handle reading for a TCP socket 
void handleClientOriginal(int new_socket, struct sockaddr_in address,std::string sessionToken,const Config& appConfig,Logger * logger) {
    std::cout << "Connection from " << inet_ntoa(address.sin_addr) << " established." << std::endl;
    logger->log("Connection from " + std::string(inet_ntoa(address.sin_addr)) + " established.");  // Log connection

    int bufferSize=appConfig.bufferSize;
    std::vector<char> buffer(bufferSize); // Start with an initial size
    int totalBytesRead = 0;
    int bytesRead = 0;

    while ((bytesRead = read(new_socket, buffer.data() + totalBytesRead, buffer.size() - totalBytesRead)) > 0) {
        totalBytesRead += bytesRead;
        // Resize buffer if needed (you may also choose to increase in chunks, e.g., another 1024 (for instance) bytes)
        if (totalBytesRead == buffer.size()) {
            buffer.resize(buffer.size() + bufferSize);
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
        //check if message is a valid isomsg
        bool isValidIsoMsg=isISO8583(data);
        if (isValidIsoMsg){
            logger->log("valid isomsg");
            
            std::string payload=data;
            checkTokenAndExecute(new_socket, sessionToken,payload,appConfig);
        }else{
            //reject if not a valid 
            logger->log("Not a valid isomsg");
            resendToRequestor(new_socket,data);
        }
    }

}
void startServerOriginal(std::string sessionToken,const Config& appConfig){
    
    Logger* logger = Logger::getInstance();
    logger->log("StartServer");
    const char* host = appConfig.host.c_str();  // Host IP address for the server (0.0.0.0 means all available interfaces)
    int port = appConfig.port;  // Port number on which the server will listen for connections
    int server_fd, new_socket;  // Socket file descriptors: one for the server, one for client connections
    struct sockaddr_in address;  // Structure to store the server's address information
    int opt = 1;  // Option value for setsockopt to enable certain socket properties
    int addrlen = sizeof(address);  // Length of the address data structure

    logger->log("StartServer 1");
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

         handleClientOriginal(new_socket, address,sessionToken,appConfig,logger);
        // Close the client socket after handling the connection
        close(new_socket);
    }

}
