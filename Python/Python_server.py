import socket
import myssn

# The server's IP address "localhost" = 127.0.0.1
HOST = "192.168.0.100"
PORT = 7 # The port used by the server

while(1):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        s.listen()
        print("Server online\nWaiting for a client...")
        conn, address = s.accept()
        
        with conn:
            print("Client connected!")
            while True:
                data = conn.recv(1024)                
                print(f"Received: {data!r}")
                if not data:
                    break
                conn.sendall(data)

    conn.close()
    print("Conexion closed")
