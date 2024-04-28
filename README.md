Device Activation and Logging Server
This repository contains a C++ application designed to handle device activation through a REST API, compute SHA-256 hashes for sequence validation, and manage network communications through a simple TCP server. The application uses various libraries such as Crypto++, cURL, and nlohmann/json for its operations.

Features
Device Activation: Communicates with a remote API to activate devices using a unique secret key.
Hash Calculation: Calculates SHA-256 hashes for given sequence values and device keys to ensure integrity and security.
TCP Server: Listens for incoming connections and handles data reception and transmission over TCP.
Logging: Logs important events and data to files, helping in debugging and record-keeping.
Requirements
C++ Compiler (C++11 or later)
Crypto++ Library
cURL Library
nlohmann/json Library
Linux Environment (developed and tested on Ubuntu)
Installation
Install Dependencies
Crypto++ Library
bash
Copy code
sudo apt-get update
sudo apt-get install libcrypto++-dev libcrypto++-doc libcrypto++-utils
cURL Library
bash
Copy code
sudo apt-get install libcurl4-openssl-dev
nlohmann/json Library
You can install it directly via the package manager or integrate it into your project as a header-only library.

bash
Copy code
sudo apt-get install nlohmann-json3-dev
Compilation
Compile the application using g++:

bash
Copy code
g++ -o device_server main.cpp -std=c++11 -lcryptopp -lcurl -lpthread
Make sure to link against the required libraries (cryptopp, curl, and potentially pthread if using multithreading).

Usage
Run the server:

bash
Copy code
./device_server
The server will start listening on the specified port for incoming TCP connections.

Code Structure
log_to_file: Logs messages to a dynamically named file based on the current date.
activateDevice: Sends a device activation request to a remote API.
calculateHash: Computes a SHA-256 hash from a device key and a sequence value.
sendSequenceHash: Submits the computed hash to a remote server for validation.
sendPlainText: Sends plain text data to a specified API endpoint using cURL.
is_valid_access_token: Validates an access token based on its expiry time.
main: Initializes a TCP server that handles incoming connections and data transmissions.