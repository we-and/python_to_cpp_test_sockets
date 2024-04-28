import requests
import json

from decouple import config


def log_to_file(text):
    log_file_name = f"log-{datetime.now().date()}.txt"
    with open(log_file_name, "a") as log_file:
        log_file.write(f"{datetime.now()} - {text}\n")


def activateDevice(secret):
    log_to_file(f'Activating device... Secret: {secret}')
    # url = "https://rposd.cullinangroup.net/api/device/activateDevice?Secret={}".format(secret)
    url = "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/device/activateDevice?Secret={}".format(secret)
    log_to_file(f'URL: {url}')

    payload = {}
    headers = {}
    log_to_file(f'Payload: {payload}')
    log_to_file(f'Headers: {headers}')

    response = requests.request("POST", url, headers=headers, data=payload)

    print(response.text)
    log_to_file(f'Response: {response.text}')
    log_to_file(f'Response JSON: {response.json()}')
    log_to_file(f'Device Activation End... Secret: {secret}')
    return response.json()


def calculateHash(currentSequenceValue, deviceKey):
    log_to_file(f'Calculating hash... Current Sequence Value: {currentSequenceValue}, Device Key: {deviceKey}')
    # get int from currentSequenceValue
    currentSequenceValue = int(currentSequenceValue)

    nextSequenceValue = currentSequenceValue + 1
    data_to_hash = deviceKey + str(nextSequenceValue)
    log_to_file(f'Data to hash: {data_to_hash}')
    sequenceHash = hashlib.sha256(data_to_hash.encode()).hexdigest()
    log_to_file(f'Sequence Hash: {sequenceHash}')
    return sequenceHash


def sendSequenceHash(deviceId, deviceSequence, deviceKey, sequenceHash, ):
    log_to_file(
        f'Sending sequence hash... Device ID: {deviceId}, Device Sequence: {deviceSequence}, Device Key: {deviceKey}, Sequence Hash: {sequenceHash}')
    # print(deviceId,deviceSequence, deviceKey, sequenceHash)
    # url = "https://rposd.cullinangroup.net/api/device/session?deviceId={}&deviceSequence={}&deviceKey={}&sequenceHash={}".format(
    #     deviceId, deviceSequence, deviceKey, sequenceHash)
    url = "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/device/session?deviceId={}&deviceSequence={}&deviceKey={}&sequenceHash={}".format(
        deviceId, deviceSequence, deviceKey, sequenceHash)
    log_to_file(f'URL: {url}')

    # print(url)
    payload = {}
    headers = {}
    log_to_file(f'Payload: {payload}')
    log_to_file(f'Headers: {headers}')
    response = requests.request("POST", url, headers=headers, data=payload)
    log_to_file(f'Response: {response.text}')
    log_to_file(f'Response JSON: {response.json()}')
    return response.json()


def sendPlainText(access_token, payload):
    log_to_file(f'Sending plain text... Access Token: {access_token}, Payload: {payload}')
    # url = "https://rposd.cullinangroup.net/7-connect/"
    url = "https://dev.bit.cullinangroup.net:5443/bit-dps-webservices/posCommand"
    log_to_file(f'URL: {url}')
    headers = {
        'Content-Type': 'text/plain',
        'Accept': 'text/plain',
        'Authorization': access_token
    }
    log_to_file(f'Headers: {headers}')

    payload_old = """
        <isomsg>
            <field id="0" value="0240"/>
            <field id="2" value="2335-2067-2063"/>
            <field id="3" value="20240307092353"/>
            <field id="4" value="600000"/>
            <field id="5" value="20231218053641"/>
            <field id="9" value="15EC729A3E3AC81"/>
            <field id="11" value="test_dragonpay"/>
            <field id="12" value="JKS67GG3"/>
            <field id="16" value="qwerty"/>
            <field id="21" value="67155231"/>
            <field id="22" value="1"/>
            <field id="23" value="0957"/>
            <field id="24" value="Bamban"/>
            <field id="25" value="3"/>
            <field id="55" value="1.2.0.0"/>
        </isomsg>
    """

    payload_test = """
    <isomsg>
  <!-- org.jpos.iso.packager.XMLPackager -->
  <field id="0" value="0600"/>
  <field id="2" value="2400-8513-1051"/>
  <field id="22" value="1"/>
  <field id="23" value="0297"/>
  <field id="55" value="1.2.0.0"/>
</isomsg>
    """

    log_to_file(f'Payload: {payload}')

    response = requests.post(url, data=payload, headers=headers)
    print(response.text)
    log_to_file(f'Response: {response.text}')
    # log_to_file(f'Response JSON: {response.json()}')

    if response.status_code == 200:
        print("Request successful.")
        print("Response from server:", response.text)
    elif response.status_code == 204:
        print("Request successful.")
        print("Status code:", response.status_code)
        print("Response from server:", response.text)
    else:
        print("Request failed.")
        print("Status code:", response.status_code)
        print("Error:", response.text)


# def execute(payload):
#     activation_result = activateDevice("4B8XP5CW2A")
#
#     nextHash = calculateHash(activation_result['deviceSequence'], activation_result['deviceKey'])
#     # print(nextHash)
#     print(f'Next Hash: {nextHash}')
#
#     session = sendSequenceHash(activation_result['deviceId'], activation_result['deviceSequence'],
#                                activation_result['deviceKey'], nextHash)
#     print(session)
#     payload = """
#         <isomsg>
#       <!-- org.jpos.iso.packager.XMLPackager -->
#       <field id="0" value="0600"/>
#       <field id="2" value="2408-5000-0031"/>
#       <field id="22" value="1"/>
#       <field id="23" value="0297"/>
#       <field id="55" value="1.2.0.0"/>
#     </isomsg>
#         """
#     sendPlainText(session['accessToken'], payload)

def is_valid_access_token():
    access_token = os.getenv("ACCESS_TOKEN")
    expiration_time_str = config("TOKEN_EXPIRY_TIME")

    if access_token and expiration_time_str:
        expiration_time = datetime.strptime(expiration_time_str, "%Y-%m-%d %H:%M:%S")
        current_time = datetime.now()
        if current_time < expiration_time:
            return True
    return False
def execute(payload):
    if is_valid_access_token():
        print("Valid access token found. Skipping activation and session request.")
        access_token = os.getenv("ACCESS_TOKEN")
    else:
        activation_result = activateDevice("4B8XP5CW2A")

        nextHash = calculateHash(activation_result['deviceSequence'], activation_result['deviceKey'])
        print(f'Next Hash: {nextHash}')

        session = sendSequenceHash(activation_result['deviceId'], activation_result['deviceSequence'],
                                   activation_result['deviceKey'], nextHash)
        access_token = session.get('accessToken', None)
        if access_token:
            # Save access token and its expiry time in environment variables
            config('ACCESS_TOKEN', access_token)
            expiration_time = datetime.now() + timedelta(seconds=session.get('expiresIn', 0))
            config('EXPIRATION_TIME', expiration_time.strftime('%Y-%m-%d %H:%M:%S'))

    # If access token is available, send the payload
    if access_token:
        sendPlainText(access_token, payload)
    else:
        print("Access token not available.")


# Define the host and port on which the server will listen
host = '0.0.0.0'  # localhost
port = 6000

# Create a socket object
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the host and port
server_socket.bind((host, port))

secret = config('SECRET', default=None)


if secret:
    print(f"Secret: {secret}")
    log_to_file(f"Secret: {secret}")
    exit(1)

else:
    print("Secret not found")
    secret = input("Please enter the secret:")
#     add into .env file
#     SECRET=4B8XP5CW2A
    with open('.env', 'a') as f:
        f.write(f"SECRET={secret}\n")



# Listen for incoming connections
server_socket.listen(5)  # parameter is the maximum number of queued connections

print(f"Server is listening on {host}:{port}")

while True:
    # Accept incoming connection
    client_socket, client_address = server_socket.accept()
    print(f"Connection from {client_address} established.")
    log_to_file("---------------------------------------------")
    log_to_file(f"Connection from {client_address} established.")

    # Receive data from the client
    data = client_socket.recv(1024).decode('utf-8')
    if not data:
        break  # If no data received, break the loop

    print(f"Received data from client: {data}")
    log_to_file(f"Received data from client: {data}")

    execute(data)

    # Echo the received data back to the client
    client_socket.sendall(data.encode('utf-8'))

    # Close the connection with the client
    client_socket.close()

# Close the server socket
server_socket.close()