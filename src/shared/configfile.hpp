#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#include <string>
struct ConfigFile {
    int port;
    std::string baseURL;
    std::string logsDir;
    std::string appDir;
    std::string serverExecutable;
    std::string deviceSecurityParametersPath;
};

#endif