#include <iostream>
#include <fstream>
#include <string>
#include <sys/stat.h> 

#include <filesystem>
namespace fs = std::filesystem;

std::string posDirectory="/root/pos/";

void create_folder(){
     // Check if the directory exists
    if (!fs::exists(posDirectory)) {
        // Create the directory
        if (fs::create_directory(posDirectory)) {
            std::cout << "Directory created successfully." << std::endl;
        } else {
            std::cout << "Failed to create directory." << std::endl;
        }
    } else {
        std::cout << "Directory already exists." << std::endl;
    }
}
int main(int argc, char* argv[]) {
    // Check if a command line argument is provided
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <POS_TOKEN>" << std::endl;
        return 1;
    }

    // Get the POS token from command line argument
    std::string posToken = argv[1];

    // Define the file path
    std::string filePath = posDirectory+"one_time_pos_token.txt";

    // Open a file in write mode
    std::ofstream outFile(filePath);

    if (!outFile) {
        std::cerr << "Error: Unable to open file at " << filePath << std::endl;
        return 1;
    }

    // Write the POS token to the file
    outFile << posToken << std::endl;

    // Close the file
    outFile.close();

 // Set file permissions to read and write for owner only
    chmod(filePath.c_str(), S_IRUSR | S_IWUSR);


    std::cout << "Token saved to " << filePath << std::endl;

    return 0;
}