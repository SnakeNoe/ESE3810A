import socket

PACKAGES = 8
HOST = "192.168.0.102" # The server's IP address
PORT = 7 # The port used by the server
msg = [b"0", b"1", b"22", b"333", b"4444", b"55555", b"666666", b"7777777"]
data = []
hex_data: int = []

key = b'\x01\x02\x03\x04\x05\x06\x07\x08\x09\x00\x01\x02\x03\x04\x05\x06'
iv = b'\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00'

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
