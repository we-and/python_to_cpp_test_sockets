#include "config.hpp"

Config::Config()
    : posDirectory("/root/pos/"),
      mainAppName("pos"),
      secretTokenFilename("secrettoken.txt"),
      activationResultFilename("device_security_parameters.json") {}

std::string Config::getPosDirectory() const {
    return posDirectory;
}

std::string Config::getMainAppName() const {
    return mainAppName;
}

std::string Config::getSecretTokenFilename() const {
    return secretTokenFilename;
}

std::string Config::getActivationResultFilename() const {
    return activationResultFilename;
}

void Config::setPosDirectory(const std::string& dir) {
    posDirectory = dir;
}

void Config::setSecretTokenFilename(const std::string& filename) {
    secretTokenFilename = filename;
}

void Config::setActivationResultFilename(const std::string& filename) {
    activationResultFilename = filename;
}