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

int removeTokenIfExists(const Config& appConfig){

    // Define the file path
    std::string filePath = appConfig.getPosDirectory()+"secrettoken.txt";
    // Check if file exists and delete it if it does
    if (fs::exists(filePath)) {
        // Try to remove the file
        bool removed = fs::remove(filePath);
        if (removed) {
            std::cout << "File deleted successfully." << std::endl;
        } else {
            std::cout << "Failed to delete the file." << std::endl;
        }
    } else {
        std::cout << "File does not exist." << std::endl;
    }

}
void createFolder(std::string dir){
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

/**
 * Saves a secret token to a file with restricted permissions.
 * 
 * The function retrieves the secret token from user input,
 * saves it to a designated file within the application's configuration directory,
 * and sets the file permissions to restrict access to the owner only.
 *
 * @param appConfig The configuration object which includes the application's directory paths.
 * @return int Returns 0 if the token is successfully saved, otherwise returns 1.
 */
int saveSecretToken(const Config& appConfig){

    std::cout << "Check if token exists"<<std::endl;
    removeTokenIfExists(appConfig);


    // Define the file path
    std::string filePath = appConfig.getPosDirectory()+"secrettoken.txt";

    auto input=inputSecretToken();
    // Open a file in write mode
    std::ofstream outFile(filePath);

    if (!outFile) {
        std::cerr << "Error: Unable to open file at " << filePath << std::endl;
        return 1;
    }

    // Write the secret token to the file
    outFile << input << std::endl;

    // Close the file
    outFile.close();

 // Set file permissions to read and write for owner only
    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);
    std::cout << "Token saved to " << filePath << std::endl;

    return 0;
}

int createPosFolder() {
    std::string folderPath = "/root/pos";
 // Check if the directory exists
    if (!fs::exists(folderPath)) {
        // Create the directory since it does not exist
        if (fs::create_directory(folderPath)) {
            std::cout << "Directory created successfully." << std::endl;
        } else {
            std::cout << "Failed to create directory." << std::endl;
        }
    } else {
        std::cout << "Directory already exists." << std::endl;
    }


    return 0;
}

// Function to execute a systemd command
bool executeSystemdCommand(const std::string& command) {
    int result = system(command.c_str());
    if (result != 0) {
        std::cerr << "Command failed: " << command << std::endl;
        return false;
    }
    return true;
}

// Main function to reload systemd, enable and start a service, and then check its status
bool reloadSystemdService(const std::string& serviceName) {
    // Sanitize the input to prevent injection attacks
    for (char c : serviceName) {
        if (!(isalnum(c) || c == '-' || c == '_')) {
            std::cerr << "Invalid service name: " << serviceName << std::endl;
            return false; // Return failure on invalid input
        }
    }

    std::string reloadCmd = "sudo systemctl daemon-reload";
    std::string enableCmd = "sudo systemctl enable " + serviceName;
    std::string startCmd = "sudo systemctl start " + serviceName;
    std::string statusCmd = "sudo systemctl status " + serviceName + " --no-pager";

    if (!executeSystemdCommand(reloadCmd)) return false;
    if (!executeSystemdCommand(enableCmd)) return false;
    if (!executeSystemdCommand(startCmd)) return false;
    
    // Execute the status command and directly output its result to the terminal
    std::cout << "Current status of the service:" << std::endl;
    return executeSystemdCommand(statusCmd);
}


/**
 * Main entry point for the application which handles initializing configurations,
 * saving a secret token, and setting up a service based on the application configuration.
 * 
 * The main function performs the following steps:
 * - Initializes the application configuration.
 * - Retrieves a secret token from the user.
 * - Saves the secret token securely.
 * - Sets up the main service of the application.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line arguments.
 * @return int Returns 0 upon successful completion of all tasks.
 */
int main(int argc, char* argv[]) {
    Config appConfig; 
    auto secretToken=inputSecretToken();

    std::cout << "Setting up server"<<std::endl;
    createPosFolder();


    std::cout << "Saving secret token"<<std::endl;
    saveSecretToken(appConfig);    ;
    std::cout << "Saving service"<<std::endl;
    setupService(appConfig.getMainAppName());

    std::cout << "Reloading service"<<std::endl;
    reloadSystemdService("pos.service");
    return 0;
}