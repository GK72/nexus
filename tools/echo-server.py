#!/usr/bin/env python3

import argparse

import socket
import sys

from dataclasses import dataclass

DESCRIPTION="""
An echo server
"""

BUFFER_SIZE = 1024

@dataclass
class ServerConfig:
    host: str
    port: int


def start_server(server: ServerConfig):
    ingress_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ingress_socket.bind((server.host, server.port))
    ingress_socket.listen(1)

    ingress_conn, addr = ingress_socket.accept()

    try:
        while True:
            data = ingress_conn.recv(BUFFER_SIZE)
            if not data:    # EOF
                break

            ingress_conn.sendall(data)
    except Exception as ex:
        pass
    finally:
        ingress_conn.close()
        ingress_socket.close()


def parse_arguments():
    parser = argparse.ArgumentParser(
        prog="Proxy server",
        description=DESCRIPTION,
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument("-p", "--port", type=int, default=4300)
    parser.add_argument(      "--host", default="localhost")

    return parser.parse_args()


def main():
    args = parse_arguments()
    server = ServerConfig(args.host, args.port)

    start_server(server)


if __name__ == "__main__":
    main()
