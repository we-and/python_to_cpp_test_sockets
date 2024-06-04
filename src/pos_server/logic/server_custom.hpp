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

#include <cstring>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <vector>

struct ClientState {
    int socket;
    std::string buffer;

    ClientState(int sock) : socket(sock) {}
};

void processNewData(char *newData, size_t newDataLength, Logger *logger)
{
    // Process the data starting at 'newData' and having length 'newDataLength'
    // For example, this could parse messages, log data, etc.
    std::string receivedData(newData, newDataLength); // Construct string from new data
    logger->log("Received new data: ");
    logger->log(receivedData);
    // Further processing...
}

void handleClientCustom(int new_socket, struct sockaddr_in address, const std::string &initialSessionToken, const Config &appConfig, Logger *logger)
{
    std::cout << "Connection from " << inet_ntoa(address.sin_addr) << " established." << std::endl;
    logger->log("Connection from " + std::string(inet_ntoa(address.sin_addr)) + " established."); // Log connection
    fd_set read_fds;
    std::vector<char> buffer(1024);
    std::string message_buffer;

    // Set socket to non-blocking mode
    fcntl(new_socket, F_SETFL, O_NONBLOCK);

    while (true) {
        FD_ZERO(&read_fds);
        FD_SET(new_socket, &read_fds);
        struct timeval timeout = {1, 0}; // 1 second timeout

        int activity = select(new_socket + 1, &read_fds, NULL, NULL, &timeout);

        if (activity < 0) {
            logger->log("Select error");
            break;
        }

        if (FD_ISSET(new_socket, &read_fds)) {
            int bytes_read = read(new_socket, buffer.data(), buffer.size());
            if (bytes_read > 0) {

                    logger->log("buffer before");
                                        logger->log(message_buffer);

                message_buffer.append(buffer.data(), bytes_read);
                    logger->log("buffer after");
                                        logger->log(message_buffer);

                size_t start_pos = message_buffer.find("<isomsg>");
                size_t end_pos = message_buffer.find("</isomsg>");

                while (start_pos != std::string::npos && end_pos != std::string::npos) {
                    std::string complete_message = message_buffer.substr(start_pos, end_pos - start_pos + 9);
                    logger->log("Complete message <isomsg>");
                   
                        std::string data=complete_message;
                        logger->log("Received data from client: " + data);  // Log received data
                        std::string cleanpayload=removeNewLines(data);
                            logger->log("cleanpayload"+cleanpayload);
                        
                        // Process the data...
                        // If data was received, echo it back to the client
                        if (isISO8583(cleanpayload)) {
                            std::string payload = cleanpayload;
                            logger->log("valid isomsg");
                            logger->log("retrieve latest sessiontoken from env");
                            const char *last_access_token = std::getenv("ACCESS_TOKEN");
                            std::string lastAccessTokenStr=std::string(last_access_token);
                            
                            checkTokenAndExecute(new_socket, lastAccessTokenStr, payload, appConfig);
                        } else {
                            logger->log("Not a valid isomsg");
                            // Reject if not valid
                            resendToRequestor(new_socket, data);
                        }


                    message_buffer.erase(0, end_pos + 9);
                    start_pos = message_buffer.find("<isomsg>");
                    end_pos = message_buffer.find("</isomsg>");
                }

                if (!message_buffer.empty() && message_buffer.find("<isomsg>") != 0) {
                      logger->log("buffer hasnt <isomsg> resend");  // Log received data
                 
                     resendToRequestor(new_socket, message_buffer);
                    message_buffer.clear();
                }
            } else if (bytes_read == 0) {
                // Connection closed by client
                break;
            } else if (errno != EWOULDBLOCK && errno != EAGAIN) {
                logger->log("Read error");
                break;
            }
        }
    }

    close(new_socket);

}
void startServerCustom(std::string initialSessionToken, const Config &appConfig)
{
    Logger *logger = Logger::getInstance();
    logger->log("StartServer");
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    int port = appConfig.port; // Port number on which the server will listen for connections

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Forcefully attaching socket to the port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    while (true)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        std::thread(handleClientCustom, new_socket, address, initialSessionToken, std::ref(appConfig), logger).detach();
    }

    close(server_fd);
}