#include "config.hpp"

Config::Config()
    : posDirectory("/root/pos/"),
      secretTokenFilename("secrettoken.txt")    {}

std::string Config::getPosDirectory() const {
    return posDirectory;
}


std::string Config::getSecretTokenFilename() const {
    return secretTokenFilename;
}


void Config::setPosDirectory(const std::string& dir) {
    posDirectory = dir;
}

void Config::setSecretTokenFilename(const std::string& filename) {
    secretTokenFilename = filename;
}
fs::path Config::getSecretTokenPath(){
    auto posDirectory=appConfig.getPosDirectory();
    auto secretTokenFilename=appConfig.getSecretTokenFilename();

    fs::path secretTokenPath = posDirectory + secretTokenFilename;
    return secretTokenPath;

}
