#!/usr/bin/env python3
import socket

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind(('127.0.0.1', 10000))

while True:
    message, address = server_socket.recvfrom(1024)
    print("[recv] {}".format(message))

