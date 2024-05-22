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


// The execute function orchestrates the communication with the API.
void checkTokenAndExecute(int requestorSocket, std::string sessionToken, std::string payload,const Config& appConfig) {
    Logger* logger = Logger::getInstance();
    logger->log("execute");  // Log the action of echoing data

    //if the remote server endpoint determines the Session Token is not valid it will send an
    //ISO8583 response with the response code "TOKEN EXPIRY" in the ISO8583 message
    //response located in field number 32 prior to returning the ISO8583 message response to
    //the requestor
    auto isValidToken=is_valid_access_token( requestorSocket);
    if (!isValidToken){
        //redo setup for getting a new access token
        auto [isNewSetupValid,newAccessToken]=setup(appConfig);
        //if new setup is valid, process the request
        if (isNewSetupValid){
            sendPlainText(requestorSocket,newAccessToken, payload,appConfig);
        }else{
            //if failed again, return to user
            resendToRequestor( requestorSocket,payload);
        }
    }else{
    // Sends the XML payload using the session's access token.
        sendPlainText(requestorSocket,sessionToken, payload,appConfig);

    }
}

bool isISO8583(const std::string& str) {
    // Constants for the start and end tags
    const std::string startTag = "<isomsg>";
    const std::string endTag = "</isomsg>";

    // Check if the string starts with the start tag and ends with the end tag
    if (str.substr(0, startTag.length()) == startTag &&
        str.substr(str.length() - endTag.length(), endTag.length()) == endTag) {
        return true;
    }

    return false;
}

// Dynamic Buffer Resizing to handle reading for a TCP socket 
void handleClient(int new_socket, struct sockaddr_in address,std::string sessionToken,const Config& appConfig,Logger * logger) {
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
/**
 * Runs a TCP echo server.
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

void startServer(std::string sessionToken,const Config& appConfig){
    //start server, choosing mode depending on the value in settings.ini
    if(appConfig.serverDispatchMode=="original"){
        return startServerOriginal(sessionToken,appConfig);
    }else if (appConfig.serverDispatchMode=="threads"){
        return startServerThreads(sessionToken,appConfig);
    }else if(appConfig.serverDispatchMode=="custom"){
        return startServerCustom(sessionToken,appConfig);
        
    }else if(){

    }
}
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
                checkTokenAndExecute(new_socket, sessionToken, payload, appConfig);
            } else {
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

    Logger logger;
    Config appConfig;
    std::string sessionToken = "your_session_token";

    while (true) {
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        std::thread(handleClient, new_socket, address, sessionToken, std::ref(appConfig), &logger).detach();
    }

    close(server_fd);
    return 0;
}
/*
void startServerCustom1(std::string sessionToken,const Config& appConfig){
     int server_fd, new_socket, client_socket[30], max_clients = 30, activity, i, valread, sd;
    int max_sd;
    struct sockaddr_in address;
    int bufferSize=appConfig.bufferSize;
    char buffer[bufferSize+1];  // data buffer of 1K + 1 to accomodate for null terminator

    // set of socket descriptors
    fd_set readfds;

    // initialise all client_socket[] to 0 so not checked
    for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }

    // create a master socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // set master socket to allow multiple connections, this is just a good habit, not necessary for select()
    setNonBlocking(server_fd);

    // type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // bind the socket to localhost port 
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);

    // try to specify maximum of 3 pending connections for the master socket
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    int addrlen = sizeof(address);
    puts("Waiting for connections ...");

    while (true) {
        // clear the socket set
        FD_ZERO(&readfds);

        // add master socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // add child sockets to set
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            // if valid socket descriptor then add to read list
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }

            // highest file descriptor number, need it for the select function
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // wait for an activity on one of the sockets, timeout is NULL, so wait indefinitely
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if ((activity < 0) && (errno != EINTR)) {
            printf("select error");
        }

        // If something happened on the master socket, then it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                continue;
            }

            printf("New connection: socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

            // add new socket to array of sockets
            for (i = 0; i < max_clients; i++) {
                if (client_socket[i] == 0) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n", i);
                    break;
                }
            }
        }

        // Else it's some IO operation on some other socket
        for (i = 0; i < max_clients; i++) {
            sd = client_socket[i];

            if (FD_ISSET(sd, &readfds)) {
                // Check if it was for closing, and also read the incoming message
                valread = read(sd, buffer, bufferSize);
                if (valread == 0) {
                    // Somebody disconnected, get his details and print
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    printf("Host disconnected, ip %s, port %d \n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));

                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    client_socket[i] = 0;
                } else {
                    // Echo back the message that came in
                    buffer[valread] = '\0';
                    send(sd, buffer, strlen(buffer), 0);
                }
            }
        }
    }
}*/
void startServerThreads(std::string sessionToken,const Config& appConfig){
    Logger* logger = Logger::getInstance();
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
        std::thread clientThread(handleClient, new_socket, address, sessionToken, appConfig, logger);
        clientThread.detach(); // Detach thread to handle client independently
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

         handleClient(new_socket, address,sessionToken,appConfig,logger);
        // Close the client socket after handling the connection
        close(new_socket);
    }

}
