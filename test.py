import socket

# Construire la requête manuellement
request = (
    "POST /test HTTP/1.1\r\n"
    "Host: 127.0.0.1:8002\r\n"
    "Content-Type: application/json\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Connection: close\r\n"
    "\r\n"
    "4\r\n"
    "test\r\n"
    "0\r\n"
    "\r\n"
)

# Envoyer via un socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect(("127.0.0.1", 8002))
    s.sendall(request.encode())

    # Lire la réponse
    response = b""
    while True:
        data = s.recv(4096)
        if not data:
            break
        response += data

print(response.decode())
