#include "config.hpp"

#include <filesystem>
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
std::fs::path Config::getSecretTokenPath(){
    auto posDirectory=this->getPosDirectory();
    auto secretTokenFilename=this->getSecretTokenFilename();

    std::fs::path secretTokenPath = posDirectory + secretTokenFilename;
    return secretTokenPath;

}
