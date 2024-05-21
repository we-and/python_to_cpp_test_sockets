#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h> 

#include <filesystem>

#include "config.hpp"
#include "inifile.hpp"

namespace fs = std::filesystem;


void chmod777(const std::string& filePath) {
    std::error_code ec; // To handle potential errors without exceptions

    // Set the permissions to 777
    fs::permissions(filePath, 
                    fs::perms::owner_all | 
                    fs::perms::group_all | 
                    fs::perms::others_all,
                    fs::perm_options::replace, 
                    ec);

    if (ec) {
        // If an error occurred, print the error message
        std::cerr << "Error setting permissions: " << ec.message() << std::endl;
    } else {
        std::cout << "Permissions set to 777 for: " << filePath << std::endl;
    }
}

std::string getAppPath(const std::string& appName){
    // Get the current working directory
    fs::path currentPath = fs::current_path();

 // Navigate up to the parent of the current path, then to 'mypath'
    fs::path basePath = currentPath.parent_path().parent_path();


    // Form the path for ExecStart
    fs::path execStartPath = basePath / "dist" / appName;

    // Construct mypath string
    std::string mypath = execStartPath.string();
    return mypath;
}
void setupService(const std::string& apppath,const std::string& appdir,const std::string& configFilePath) {
    // Get the current working directory
    

    std::string apppathwithargs = apppath + " -f "+configFilePath;
    

    fs::path appdirPath = appdir;
    // Name of the service file
    std::string serviceFilename = "pos.service";
    fs::path serviceFilePath = appdirPath / serviceFilename;

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
    serviceFile << "User=ubuntu\n";
    serviceFile << "ExecStart=" << apppathwithargs << "\n";
    serviceFile << "Restart=no\n\n";
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

void grantRightsToApp(std::string appPath){
    std::cout << "Grant app rights to" <<appPath<<std::endl;
        // Copy the service file to /etc/systemd/system
    std::string copyCmd = "sudo setcap 'cap_dac_override=eip' "+appPath;
    int result = system(copyCmd.c_str());
    if (result != 0) {
        std::cerr << "Failed to give app rights to"<<appPath << std::endl;
    } else {
        std::cout << "App rights set successfully." << std::endl;
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
    return 0;
}
int createFolder(std::string dir){
     // Check if the directory exists
    if (!fs::exists(dir)) {
        // Create the directory
        if (fs::create_directory(dir)) {
            std::cout << "Directory created successfully." << std::endl;
    return 0;
        } else {
            std::cout << "Failed to create directory." << std::endl;
            return 1;

        }
    } else {
        std::cout << "Directory already exists." << std::endl;
    return 0;
    }
}

// Function to prompt user and get a string
std::string getUserInput(const std::string& prompt) {
    std::string input;
    std::cout << prompt;
    std::getline(std::cin, input); // Use getline to read the whole line
    return input;
}

bool isFullPath(const std::string& path) {
    // Check if path contains any directory delimiters
    return path.find('/') != std::string::npos || path.find('\\') != std::string::npos;
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
    chmod777(filePath);
    std::cout << "Token saved to " << filePath << std::endl;

    return 0;
}

int createPosFolder() {
    std::string folderPath = "/root/pos";
     bool exists=fs::exists(folderPath);
    
 // Check if the directory exists
    if (!fs::exists(folderPath)) {
     std::cout << "createPosFolder folder not existing" << std::endl;
        // Create the directory since it does not exist
        if (fs::create_directory(folderPath)) {
            std::cout << "Pos directory created successfully." << std::endl;
        } else {
            std::cout << "Failed to create pos directory." << std::endl;
        }
    } else {
        std::cout << "Pos directory already exists." << std::endl;
    }


    return 0;
}
int createPosLogsFolder() {
    std::string folderPath = "/home/ubuntu/pos/logs";
     bool exists=fs::exists(folderPath);
    
 // Check if the directory exists
    if (!fs::exists(folderPath)) {
     std::cout << "createPosFolder folder not existing" << std::endl;
        // Create the directory since it does not exist
        if (fs::create_directory(folderPath)) {
            std::cout << "Pos directory created successfully." << std::endl;
        } else {
            std::cout << "Failed to create pos directory." << std::endl;
        }
    } else {
        std::cout << "Pos directory already exists." << std::endl;
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

    if (!executeSystemdCommand(reloadCmd)) return false;
    if (!executeSystemdCommand(enableCmd)) return false;
    return true;
}




// Main function to reload systemd, enable and start a service, and then check its status
bool startSystemdService(const std::string& serviceName) {
    // Sanitize the input to prevent injection attacks
    for (char c : serviceName) {
        if (!(isalnum(c) || c == '-' || c == '_')) {
            std::cerr << "Invalid service name: " << serviceName << std::endl;
            return false; // Return failure on invalid input
        }
    }

    std::string startCmd = "sudo systemctl start " + serviceName;
    std::string statusCmd = "sudo systemctl status " + serviceName + " --no-pager";

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
    std::cout << "SET TOKEN version 0.8"<<std::endl;
    std::string configFilePath;
    bool restartServer=true;
    // Parse command-line arguments
    if (argc==1){
            std::cerr << "Usage: " << argv[0] << " -f <config_file_path>" << std::endl;
            return 1;
    }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) { // Make sure we do not go out of bounds
            configFilePath = argv[++i]; // Increment 'i' to skip the file path in the next loop iteration
        }else if (arg == "--nostart") {
            restartServer = false; // Set noStart flag
        }  else {
            std::cerr << "Usage: " << argv[0] << " -f <config_file_path>" << std::endl;
            return 1;
        }
    }
    
    
    //search in pos folder if not a full abs path
    if (!isFullPath(configFilePath)){
        configFilePath="/home/ubuntu/pos/conf/"+configFilePath;
    }
    //read config ini file
    auto [readConfigResult,configFile]=readIniFile(configFilePath);
    if (readConfigResult>0){
            std::cerr << "Config file not found at  "<< configFilePath<<". Check within /home/ubuntu/pos/conf or try with an absolute path." << std::endl;
        return 1;
    }

    
    std::cout << "Config file             : "<<configFilePath<<std::endl;
    std::cout << "Start server at the end : "<<restartServer<<std::endl;
    std::cout << "Creating folders"<<std::endl;
    
    createPosFolder();
    createPosLogsFolder();


    Config appConfig; 
    std::string apppath=configFile.appDir+configFile.serverExecutable;
    std::cout << "POS Server path         : "<<apppath<<std::endl;
    grantRightsToApp(apppath);

    std::cout << "Ask Token"<<std::endl;
    saveSecretToken(appConfig);    ;
    std::cout << "Saving service"<<std::endl;
    setupService(apppath,configFile.appDir, configFilePath);

    std::cout << "Reloading service manager"<<std::endl;
    reloadSystemdService("pos");
    if(restartServer){
        startSystemdService("pos");
    }
    return 0;
}