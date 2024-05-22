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



void read_cb(struct bufferevent *bev, void *ctx) {
     CallbackData* cbData = static_cast<CallbackData*>(ctx);
    Logger* logger = cbData->logger;
    int buffer_size = cbData->buffer_size;


    char buffer[buffer_size];
    std::string data;
    while (true) {
        int n = bufferevent_read(bev, buffer, sizeof(buffer));
        if (n <= 0) break;
        data.append(buffer, n);
    }
    logger->log("Received data from client: " + data);

    if (!data.empty()) {
        if (isISO8583(data)) {
            std::string payload = data;
            checkTokenAndExecute(bev, "your_session_token", payload, Config());
        } else {
            resendToRequestor(bev, data);
        }
    }
}

void event_cb(struct bufferevent *bev, short events, void *ctx) {
    if (events & BEV_EVENT_ERROR) {
        perror("Error from bufferevent");
    }
    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        bufferevent_free(bev);
    }
}

void accept_conn_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *address, int socklen, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, NULL, event_cb, ctx);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
    Logger *logger = static_cast<Logger*>(ctx);
    logger->log("Connection established");
}

void accept_error_cb(struct evconnlistener *listener, void *ctx) {
    struct event_base *base = evconnlistener_get_base(listener);
    int err = EVUTIL_SOCKET_ERROR();
    fprintf(stderr, "Got an error %d (%s) on the listener. Shutting down.\n", err, evutil_socket_error_to_string(err));
    event_base_loopexit(base, NULL);
}


void startServerLibevent(std::string sessionToken,const Config& appConfig){
       Logger* logger = Logger::getInstance();
    logger->log("StartServer");
   


    int buffer_size = appConfig.bufferSize;  // Example buffer size
    CallbackData cbData = { logger, buffer_size };

    struct event_base *base;
    struct evconnlistener *listener;
    struct sockaddr_in sin;
   
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(8080);

    listener = evconnlistener_new_bind(base, accept_conn_cb, (void*)&cbData,
        LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1,
        (struct sockaddr*)&sin, sizeof(sin));

    if (!listener) {
        perror("Could not create a listener!");
        return 1;
    }
    evconnlistener_set_error_cb(listener, accept_error_cb);

    event_base_dispatch(base);

    evconnlistener_free(listener);
    event_base_free(base);
    return 0;
}
