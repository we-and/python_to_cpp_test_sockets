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
void handleClientThreads(int new_socket, struct sockaddr_in address,std::string sessionToken,const Config& appConfig,Logger * logger) {
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
            std::string payload=data;
            checkTokenAndExecute(new_socket, sessionToken,payload,appConfig);
        }else{
            //reject if not a valid 
            resendToRequestor(new_socket,data);
        }
    }

}
void startServerThreads(std::string sessionToken,const Config& appConfig){
     Logger* logger = Logger::getInstance();
    logger->log("StartServer");
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int port = appConfig.port;  // Port number on which the server will listen for connections
   
    int addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    // Bind the socket to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Start listening to incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
            perror("accept");
            continue;
        }
        
        // Thread to handle client
        std::thread clientThread(handleClientThreads, new_socket, address, sessionToken, appConfig, logger);
        clientThread.detach(); // Detach thread to handle client independently
    }
}
