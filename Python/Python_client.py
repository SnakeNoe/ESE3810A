import socket

PACKAGES = 8
HOST = "192.168.0.102" # The server's IP address
PORT = 7 # The port used by the server

# Values to debug session
#Raw message = [b"0", b"1", b"22", b"333", b"4444", b"55555", b"666666", b"7777777"]
#First encripted message with CRC = [b"8839C1CB5B61C2E14F89F314DBCF3A997D56BEBD"]
#First encryipted message = [b"8839C1CB5B61C2E14F89F314DBCF3A99"]
msg = [b"\x889\xc1\xcb[a\xc2\xe1O\x89\xf3\x14\xdb\xcf:\x99}V\xbe\r",
       b"\x14;t\xe6rc\x1fyH\x04N\xfd.\xeb\x8d\xdfLy\xae\xa4",
       b"\x9d\\a\xd6\xa71-\xbb\xd1OC\xe2\xd3\xb7N\xd2Hs\x9b\xc1",
       b"\xb3uz\x05\xf7\xfd\xc7G&VK.\x1c\xdb\x15K\x032/\xd0",
       b"\xfa\xb0\xff\xe25]\x98b4a\x17\x93^\xb0\x8dn\x19/\xb5H",
       b"e\x96\x91\xd8\xcb]\xda\xc4*\x07\x17\xdd2e\xb0\xfb+\xa6\x87\xde",
       b"/`\x89\xb0=n\xf0\t\xf7\xeb\xcf\xee\"\x99\xdf\xc9\x16\x1f\x07\x12",
       b"\x8b\x97\x95)\xa4\xd2?\xbeu\xc0\xc4W\xe0G\xdf\x92\x88\x81\xb3h"]
data = []

for i in range(PACKAGES):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((HOST, PORT))

        # Transmit data
        print("Transmitted: ", end="")
        print(msg[i])
        s.sendall(msg[i])
        
        #Receive
        data.append(s.recv(1024))
        print(f"Received:    {data[i]!r}")
        print("")
