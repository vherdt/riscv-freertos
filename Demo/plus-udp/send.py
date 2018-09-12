#!/usr/bin/env python3
import time
import socket

client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
client_socket.settimeout(1.0)
message = b'test'
#addr = ("192.168.0.188", 10001)
#addr = ("127.0.0.1", 10000)
addr = ("134.102.218.252", 10000)

while True:
    print("[send] {}".format(message))
    client_socket.sendto(message, addr)
    time.sleep(1)
