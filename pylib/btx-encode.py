#!/usr/bin/env python3

import ctypes
import os
import sys
from ctypes.util import find_library

def main():
    # Load the library, assuming it is installed in a system-standard location.
    lib_path = find_library("btx_c")
    if not lib_path:
        # On some systems find_library might fail for non-standard paths,
        # so we also try loading it directly.
        lib_path = "libbtx_c.so"

    try:
        lib = ctypes.CDLL(lib_path)
    except OSError as e:
        print(f"Error: Could not find libbtx_c.so. Ensure it's installed and in your library path.")
        print(f"Details: {e}")
        sys.exit(1)

    # Define types to match the C API
    class BtxResult(ctypes.Structure):
        _fields_ = [
            ("data", ctypes.POINTER(ctypes.c_uint8)),
            ("size", ctypes.c_size_t),
            ("error", ctypes.c_char_p),
            ("internal", ctypes.c_void_p)
        ]

    # Set up function signatures
    lib.btx_encode.argtypes = [ctypes.c_char_p]
    lib.btx_encode.restype = BtxResult

    lib.btx_free_result.argtypes = [BtxResult]
    lib.btx_free_result.restype = None

    # Test cases
    test_inputs = [
        b"\\x48\\x65\\x6C\\x6C\\x6F",  # "Hello"
        b"\\b10101010 \\x55",         # AA 55
        b"// This is a comment\n\\xDE\\xAD\\xBE\\xEF" # DE AD BE EF
    ]

    print(f"Using library: {lib_path}\n")

    for input_btx in test_inputs:
        print(f"Input BTX: {input_btx.decode()}")

        # Call the C-binding
        result = lib.btx_encode(input_btx)

        if result.error:
            print(f"  Error: {result.error.decode()}")
        else:
            # Access the binary data
            data_bytes = ctypes.string_at(result.data, result.size)
            print(f"  Encoded size: {result.size} bytes")
            print(f"  Encoded data (hex): {data_bytes.hex(' ').upper()}")

        # Always free the result
        lib.btx_free_result(result)
        print("-" * 40)

if __name__ == "__main__":
    main()
