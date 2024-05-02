#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h> 

#include <filesystem>

#include "config.hpp"

namespace fs = std::filesystem;



void setupService(const std::string& appName) {
    // Get the current working directory
    fs::path currentPath = fs::current_path();

    // Form the path for ExecStart
    fs::path execStartPath = currentPath / appName;

    // Construct mypath string
    std::string mypath = execStartPath.string();

    // Name of the service file
    std::string serviceFilename = "pos.service";
    fs::path serviceFilePath = currentPath / serviceFilename;

    // Create and open a text file
    std::ofstream serviceFile(serviceFilePath);

    // Check if the file stream is ready for writing
    if (!serviceFile) {
        std::cerr << "Failed to create " << serviceFilePath << std::endl;
        return;
    }

    // Write the contents of the systemd service file
    serviceFile << "[Unit]\n";
    serviceFile << "Description=POS application startup script\n\n";
    serviceFile << "[Service]\n";
    serviceFile << "Type=simple\n";
    serviceFile << "ExecStart=" << mypath << "\n";
    serviceFile << "Restart=on-failure\n\n";
    serviceFile << "[Install]\n";
    serviceFile << "WantedBy=multi-user.target\n";

    // Close the file
    serviceFile.close();

    // Copy the service file to /etc/systemd/system
    std::string copyCmd = "sudo cp " + serviceFilePath.string() + " /etc/systemd/system/";
    int result = system(copyCmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to copy service file to /etc/systemd/system" << std::endl;
    } else {
        std::cout << "Service file installed successfully." << std::endl;
    }
}

void create_folder(std::string dir){
     // Check if the directory exists
    if (!fs::exists(dir)) {
        // Create the directory
        if (fs::create_directory(dir)) {
            std::cout << "Directory created successfully." << std::endl;
        } else {
            std::cout << "Failed to create directory." << std::endl;
        }
    } else {
        std::cout << "Directory already exists." << std::endl;
    }
}
// Function to prompt user and get a string
std::string getUserInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input); // Use getline to read the whole line
    return input;
}
std::string inputSecretToken(){
        // Prompt for user input
        std::string userString = getUserInput("Please enter your secret token: ");
        return userString;
}

int saveSecretToken(const Config& appConfig){

    // Define the file path
    std::string filePath = appConfig.getPosDirectory()+"secrettoken.txt";

    // Open a file in write mode
    std::ofstream outFile(filePath);

    if (!outFile) {
        std::cerr << "Error: Unable to open file at " << filePath << std::endl;
        return 1;
    }

    // Write the secret token to the file
    outFile << appConfig.getSecretTokenFilename() << std::endl;

    // Close the file
    outFile.close();

 // Set file permissions to read and write for owner only
    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);
    std::cout << "Token saved to " << filePath << std::endl;

    return 0;
}

int main(int argc, char* argv[]) {
    Config appConfig; 

    auto secretToken=inputSecretToken();
    saveSecretToken(appConfig);    ;
    setupService(appConfig.getMainAppName());

    return 0;
}