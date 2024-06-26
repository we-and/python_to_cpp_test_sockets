#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <string>
struct ConfigFile {
    int port;
    std::string baseURL;
    std::string logsDir;
    std::string appDir;
    std::string host;
    std::string envfilePath;
    std::string serverExecutable;
    std::string deviceSecurityParametersPath;
    int bufferSize;
    std::string serverDispatchMode;
};

#endif