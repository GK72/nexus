#!/usr/bin/env python3

import argparse
import logging

import socket
import sys

from dataclasses import dataclass

DESCRIPTION="""
A proxy server
"""

BUFFER_SIZE = 1024

logging.basicConfig(
    level=logging.DEBUG,
    format='%(asctime)s [%(name)s] %(levelname)s: %(message)s'
)

logger = logging.getLogger("Proxy")


@dataclass
class ServerConfig:
    host: str
    port: int


def start_proxy(server: ServerConfig, client: ServerConfig):
    ingress_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    ingress_socket.bind((server.host, server.port))
    ingress_socket.listen(1)

    ingress_conn, addr = ingress_socket.accept()
    logger.info(f"Client connected: {addr[0]}:{addr[1]}")

    try:
        egress_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        egress_socket.connect((client.host, client.port))
        logger.info(f"Proxy connected to: {client.host}:{client.port}")

        while True:
            data = ingress_conn.recv(BUFFER_SIZE)
            if not data:    # EOF
                break

            logger.debug(f"Proxy sent {len(data)} bytes")

            egress_socket.sendall(data)
            reply = egress_socket.recv(BUFFER_SIZE)

            logger.debug(f"Proxy received {len(reply)} bytes")

            if reply is not None:
                ingress_conn.sendall(reply)
    except Exception as ex:
        logger.error(f"Exception happened: {ex}")
    finally:
        ingress_conn.close()
        ingress_socket.close()
        logger.info(f"Client disconnected: {addr[0]}:{addr[1]}")

        egress_socket.close()
        logger.info(f"Proxy disconnected from: {client.host}:{client.port}")


def parse_arguments():
    parser = argparse.ArgumentParser(
        prog="Proxy server",
        description=DESCRIPTION,
        formatter_class=argparse.RawTextHelpFormatter
    )

    parser.add_argument("-pp", "--proxy-port", type=int, default=4200)
    parser.add_argument("-ph", "--proxy-host", default="localhost")
    parser.add_argument("-sp", "--server-port", type=int, default=4300)
    parser.add_argument("-sh", "--server-host", default="localhost")

    return parser.parse_args()


def main():
    args = parse_arguments()
    server = ServerConfig(args.proxy_host, args.proxy_port)
    client = ServerConfig(args.server_host, args.server_port)

    start_proxy(server, client)


if __name__ == "__main__":
    main()
