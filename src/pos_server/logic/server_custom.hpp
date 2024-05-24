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

    std::string dataBuffer; 

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
                logger->log("bytesread>0");
                 logger->log(std::to_string(bytesRead));  // Log connection
                 // Log connection
                                 logger->log("data");

                bool wasEmpty=(dataBuffer.empty()) ;

                logger->log( buffer.data());
                dataBuffer.append(buffer.data(), bytesRead);

                totalBytesRead += bytesRead;
                                logger->log( std::to_string(totalBytesRead));
                // Resize buffer if needed
                if (totalBytesRead == buffer.size()) {
                    logger->log("resize buffer");  // Log connection
                    buffer.resize(buffer.size() + bufferSize);
                }

            //if (wasEmpty){
             
            // Check if the accumulated data starts with <isomsg>
            if (  dataBuffer.find("<isomsg>") == 0) {
                                  logger->log("Data buffer starts with <isomsg>");
            
                size_t start, end;
                // Keep processing while complete messages are in the buffer
                while ((start = dataBuffer.find("<isomsg>")) != std::string::npos &&
                    (end = dataBuffer.find("</isomsg>", start)) != std::string::npos) {

                                        logger->log("msg is complete");  // Log connection

                    std::string data = dataBuffer.substr(start, end + 9 - start); // 9 is length of "</isomsg>"

                        logger->log("Received data from client: " + data);  // Log received data
                        std::string cleanpayload=removeNewLines(data);
                            logger->log("cleanpayload"+cleanpayload);
                        
                        // Process the data...
                        // If data was received, echo it back to the client
                        if (isISO8583(cleanpayload)) {
                            std::string payload = cleanpayload;
                            logger->log("valid isomsg");
                            checkTokenAndExecute(new_socket, sessionToken, payload, appConfig);
                        } else {
                            logger->log("Not a valid isomsg");
                            // Reject if not valid
                            resendToRequestor(new_socket, data);
                        }

                  logger->log("erase buffer");  // Log received data
                    dataBuffer.erase(start, end + 9 - start);
                }

            }else{
             
                if (wasEmpty){
                logger->log("not starting with <isomsg>,erase buffer");  // Log received data
                       
                resendToRequestor(new_socket, dataBuffer);
                dataBuffer.clear();
                }
           // }

            }


            } else if (bytesRead == 0) {
                std::cout << "Client disconnected" << std::endl;
                logger->log("Client disconnected"); 
                break;
            } else {
                logger->log("read failed"); 
                perror("read failed");
                break;
            }
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