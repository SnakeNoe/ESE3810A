import socket

PACKAGES = 8
HOST = "192.168.0.102" # The server's IP address
PORT = 7 # The port used by the server
#msg = [b"0", b"1", b"22", b"333", b"4444", b"55555", b"666666", b"7777777"]
msg = [b"\x889\xc1\xcb[a\xc2\xe1O\x89\xf3\x14\xdb\xcf:\x99}V\xbe\r"]
data = []

for i in range(PACKAGES):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        # Transmit data in bytes
        print("Transmitted: ", end="")
        print(msg[i])
        s.sendall(msg[i])
        # Received data in bytes 'b' but in hex format
        data.append(s.recv(1024))
        print(f"Received: {data[i]!r}")
        print("")
