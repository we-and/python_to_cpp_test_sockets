# POS Server
## Quick start

```
sudo apt-get update && sudo apt-get install -y libcrypto++-dev libcurl4-openssl-dev g++

git clone https://github.com/we-and/python_to_cpp_test_sockets  
cd python_to_cpp_test_sockets/

cd src/pos_server
g++ --std=c++17 -o ../../dist/pos main.cpp ../shared/config.cpp -I../shared -I./requests -lcryptopp -lcurl -lstdc++fs 
cd ../..

cd src/set_token
g++ --std=c++17 -o ../../dist/set_token set_token.cpp ../shared/config.cpp -I../shared -lcryptopp -lcurl -lstdc++fs
cd ../../dist

#RUN COMMANDS
#SET TOKEN
#ini file from an absolute path
sudo ./set_token -f /home/ubuntu/pos/conf/dev.ini

#ini file from /home/ubuntu/pos/conf
sudo ./set_token -f settings.ini

#use --nostart to change token without running the server 
sudo ./set_token -f settings.ini --nostart

#CHECKS
sudo ls /root/pos
sudo cat /root/pos/secrettoken.txt
sudo cat /etc/systemd/system/pos.service

#RUN
sudo ./pos -f /home/ubuntu/pos/conf/dev.ini


```

This repository contains a C++ application designed to handle device activation through a REST API, compute SHA-256 hashes for sequence validation, and manage network communications through a simple TCP server. The application uses various libraries such as Crypto++, cURL, and nlohmann/json for its operations.

![](https://github.com/we-and/python_to_cpp_test_sockets/blob/main/screenshot.png?raw=true)


## User rights
Ideally we want to deny access of secret tokens to the user but allow to app to read them. Set_token sets rights with
```
sudo setcap 'cap_dac_override=eip' ../../dist/pos
```
Rerun this command if you recompile and don't want to run set_token again

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
 * Device Activation: Communicates with a remote API to activate devices using a unique secret key.
 * Hash Calculation: Calculates SHA-256 hashes for given sequence values and device keys to ensure integrity and security.
 * TCP Server: Listens for incoming connections and handles data reception and transmission over TCP.
 * Logging: Logs important events and data to files, helping in debugging and record-keeping.

## Relies on
```
G++ Compiler (version 5.0 or later to allow for C++17)
Crypto++ Library
cURL Library
nlohmann/json Library
Linux Environment (developed and tested on Ubuntu 16.04)
```

## Dependencies for compilation
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
sudo ./set_token -f configfile.ini

# run server
./pos -f configfile.ini
```


