#echo-client.py

import socket

PACKAGES = 8
HOST = "192.168.0.102" # The server's IP address
PORT = 7 # The port used by the server
msg = [b"0", b"1", b"22", b"333", b"4444", b"55555", b"666666", b"7777777"]
data = []

for i in range(PACKAGES):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        #s.sendall(b"A")
        s.sendall(msg[i])
        #data = s.recv(1024)
        data.append(s.recv(1024))

        print(f"Received: {data[i]!r}")
