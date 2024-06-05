#ifndef SETTINGS_H
#define SETTINGS_H

#include <string>

class Config {
private:
    std::string posDirectory;
    std::string mainAppName;
    
    std::string secretTokenFilename;
    std::string activationResultFilename;
public:
    int port;
    std::string host;
    std::string baseURL;
    std::string logsDir;
    std::string deviceSecurityParametersPath;
    std::string envFilePath;
    int bufferSize;
    std::string serverDispatchMode;

public:
    // Constructor to initialize the settings
    Config();

    // Getters
    std::string getPosDirectory() const;
    std::string getSecretTokenFilename() const;
    std::string getActivationResultFilename() const;

    // Setters (if you want to allow modifications to settings)
    void setPosDirectory(const std::string& dir);
    void setSecretTokenFilename(const std::string& filename);
    void setActivationResultFilename(const std::string& filename);
    std::string getSecretTokenPath();
};

#endif // SETTINGS_H