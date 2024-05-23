import socket
import time
def test_echo_server():
    host = '127.0.0.1'  # Server's IP address
    port = 6000         # Server's port number
    message = "Hello, Server"  # Message to send
    isomessage='<isomsg><!-- org.jpos.iso.packager.XMLPackager --><field id="0" value="0600"/><field id="2" value="2454-3000-0002"/><field id="22" value="1"/><field id="23" value="0297"/><field id="55" value="1.2.0.0"/></isomsg>'

    # Create a socket object using IPv4 and TCP protocols
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((host, port))  # Connect to server
        #print(f"Connected to {host}:{port}")
        print("Connected to {0}:{1}".format(host, port)) 

        print("-------------------------")

        s.sendall(message.encode())  # Send message to server
       # print(f"Sent: {message}")
        print("Sent non-isomessage: {}".format(message)) 
        # Receive data from the server
        data = s.recv(1024)  # Buffer size is 1024 bytes
        received_message = data.decode()
        print("Received: {}".format(received_message))

        time.sleep(1)
        print("-------------------------")
        s.sendall(isomessage.encode())  # Send message to server
       # print(f"Sent: {message}")
        print("Sent isomessage: {}".format(isomessage)) 
        # Receive data from the server
        data = s.recv(1024)  # Buffer size is 1024 bytes
        received_message = data.decode()
        print("Received: {}".format(received_message))




# Run the function
if __name__ == '__main__':
    test_echo_server()