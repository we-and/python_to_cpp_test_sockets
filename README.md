# Device Activation and Logging Server
## Quick start

```
sudo apt-get update && sudo apt-get install -y libcrypto++-dev libcurl4-openssl-dev g++

git clone https://github.com/we-and/python_to_cpp_test_sockets  
cd python_to_cpp_test_sockets/

cd src/pos_server
g++ --std=c++17 -o ../../dist/pos main.cpp ../shared/config.cpp -I../shared -lcryptopp -lcurl -lstdc++fs
 
cd ../..

cd src/set_token
g++ --std=c++17 -o ../../dist/set_token set_token.cpp ../shared/config.cpp -I../shared -lcryptopp -lcurl -lstdc++fs
cd ../..


cd dist
sudo ./set_token
sudo ls /root/pos
sudo cat /root/pos/secrettoken.txt
sudo cat /etc/systemd/system/pos.service 
```

This repository contains a C++ application designed to handle device activation through a REST API, compute SHA-256 hashes for sequence validation, and manage network communications through a simple TCP server. The application uses various libraries such as Crypto++, cURL, and nlohmann/json for its operations.

![](https://github.com/we-and/python_to_cpp_test_sockets/blob/main/screenshot.png?raw=true)


## Tasks
The main function initializes a server socket, binds it to a specified host and port, listens for incoming connections,
 * and handles them by echoing back any received data. The server runs indefinitely until it encounters a failure in
 * socket operations like bind, listen, or accept.

Function Flow:
 * 1. Create a socket.
 * 2. Set socket options to reuse the address and port.
 * 3. Bind the socket to a host (IP address) and port.
 * 4. Listen on the socket for incoming connections.
 * 5. Accept a connection from a client.
 * 6. Read data from the client, log the received data, and send it back (echo).
 * 7. Close the connection and wait for another.

## Features
Device Activation: Communicates with a remote API to activate devices using a unique secret key.
Hash Calculation: Calculates SHA-256 hashes for given sequence values and device keys to ensure integrity and security.
TCP Server: Listens for incoming connections and handles data reception and transmission over TCP.
Logging: Logs important events and data to files, helping in debugging and record-keeping.

## Requirements
C++ Compiler (C++11 or later)
Crypto++ Library
cURL Library
nlohmann/json Library
Linux Environment (developed and tested on Ubuntu)

## Installation
Install Dependencies
Crypto++ Library

```
sudo apt-get update
sudo apt-get install libcrypto++-dev libcrypto++-doc libcrypto++-utils
```

cURL Library
```
sudo apt-get install libcurl4-openssl-dev
```

nlohmann/json Library
Used but no need to add dependency.

## Compilation
Compile the server using g++:

```
cd src/pos_server
g++ --std=c++17 -o ../../dist/pos main.cpp ../shared/config.cpp -I../shared -lcryptopp -lcurl
cd ../..
```
```
cd src/
g++ --std=c++17 -o ../../dist/set_token set_token.cpp ../shared/config.cpp -I../shared -lcryptopp -lcurl
cd ../..
```

Make sure to link against the required libraries (cryptopp, curl, and potentially pthread if using multithreading).

## Usage
Ask admin to run set_token first, as in:
```
#one time config
cd dist
sudo ./set_token

# run server
./pos_server
```



The server will start listening on the specified port for incoming TCP connections.

## Code Structure
 - log_to_file: Logs messages to a dynamically named file based on the current date.
 - activateDevice: Sends a device activation request to a remote API.
 - calculateHash: Computes a SHA-256 hash from a device key and a sequence value.
 - sendSequenceHash: Submits the computed hash to a remote server for validation.
 - sendPlainText: Sends plain text data to a specified API endpoint using cURL.
 - is_valid_access_token: Validates an access token based on its expiry time.
 - main: Initializes a TCP server that handles incoming connections and data transmissions.
