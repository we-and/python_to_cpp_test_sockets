import socket

def test_echo_server():
    host = '127.0.0.1'  # Server's IP address
    port = 6000         # Server's port number
    message = "Hello, Server"  # Message to send

    # Create a socket object using IPv4 and TCP protocols
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))  # Connect to server
        #print(f"Connected to {host}:{port}")
        print("Connected to {0}:{1}".format(host, port)) 
        s.sendall(message.encode())  # Send message to server
       # print(f"Sent: {message}")
        print("Sent: {0}".format(message)) 
        # Receive data from the server
        data = s.recv(1024)  # Buffer size is 1024 bytes
        received_message = data.decode()
        print(f"Received: {0}".format(received_message))


# Run the function
if __name__ == '__main__':
    test_echo_server()