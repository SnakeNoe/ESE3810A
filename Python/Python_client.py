#echo-client.py

import socket
from Crypto.Cipher import AES # For AES128

PACKAGES = 8
HOST = "192.168.0.102" # The server's IP address
PORT = 7 # The port used by the server
msg = [b"0", b"1", b"22", b"333", b"4444", b"55555", b"666666", b"7777777"]
data = []
hex_data: int = []

###############################################################################
#                                   CONFIG
###############################################################################
SERVER_PORT    = 7 # Port to accept client connections
SOCKET_TIMEOUT = 20 #seconds
# AES 128 Encryption Key and Initialization Vector
# See this Wikipedia's wiki on Block Cipher for details
# https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation
# 
key = b'\x01\x02\x03\x04\x05\x06\x07\x08\x09\x00\x01\x02\x03\x04\x05\x06'
iv = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

verbose = True # True -> Enables debug messages

###############################################################################
#                                    AES
###############################################################################
# AES128 Encryption function
def enc(message):
    # To encrypt an array its lenght must be a multiple of 16 so we add zeros
    ptxt = message + (b'\x00' * (16 - len(message) % 16))
    # Encrypt in EBC mode
    encryptor = AES.new(key, AES.MODE_CBC, IV=iv)
    ciphed_msg = encryptor.encrypt(ptxt)
    return ciphed_msg


# AES128 Decryption function
def dec(message):
    # Decrypt in EBC mode
    decryptor =  AES.new(key, AES.MODE_CBC, IV=iv)
    deciphed_msg = decryptor.decrypt(message)
    # Remove the Padding
    deciphed_msg = deciphed_msg.rstrip(b'\x00')
    return deciphed_msg

for i in range(PACKAGES):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        print("Transmitted: ", end="")
        print(msg[i])
        s.sendall(msg[i])
        #data = s.recv(1024)
        data.append(s.recv(1024))
        print(f"Received: {data[i]!r}")

        # Transform string to hexadecimal and separate by half a byte
        """data[i] = data[i].hex()
        print("Hex: " + data[i])"""

        """j = 0
        for c in range(int(len(data[i]) / 2)):
            high = 0
            low = 0

            high = (data[i][j:j+2])
            low = (data[i][j+2:j+4])
            hex_data.append(high + low)
            print(hex(int((high))), hex(int(hex_data[i])))

            j += 2
            print(hex_data[i])"""
