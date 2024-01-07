import argparse
import subprocess
import time

from typing import Tuple

import tools
from suite import Suite


def build():
    return tools.exec_command(f"cargo build")


def start_server() -> subprocess.Popen:
    ret =  tools.exec_command(f"cargo run", background=True, output="test-output.txt")
    time.sleep(0.5)
    return ret


def run_locally():
    build()
    server = start_server()

    tools.send_tcp("localhost:9999", "bla", expect_answer=False)
    server.terminate()

    tools.file_contains_line("[98, 108, 9]", "test-output.txt", verbose=True)


def run_in_docker():
    # TODO(feat-tessie): context manager to clean up container in case of exception
    container = tools.start_container("local/nexus-tcp", ports={ "9999/tcp": 9990 })
    tools.send_tcp("localhost:9990", "bla", expect_answer=False)
    result = container.logs()
    container.stop()

    tools.contains_line("[98, 108, 9]", result, verbose=True)


def run_test(test_args: dict):
    suite = Suite("Base cases")
    suite.test_case("Local run", run_locally)
    suite.test_case("Docker run", run_in_docker)
    suite.run()
