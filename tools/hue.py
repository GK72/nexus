#!/usr/bin/env python3

import os
import requests
import argparse

HUE_ADDRESS = os.getenv("HUE_ADDRESS")
HUE_USER = os.getenv("HUE_USER")
HUE_API = f"http://{HUE_ADDRESS}/api/{HUE_USER}"

LIGHT_MAP = {
    "desk": 2,
    "strip": 5,
    "go": 8
}


def get_lights():
    ret = {}
    resp = requests.get(f"{HUE_API}/lights").json()

    for id, light in resp.items():
        ret[int(id)] = (light["name"], light["state"]["on"])

    return ret


def switch_light(idx, state):
    return requests.put(f"{HUE_API}/lights/{idx}/state", json={"on": state}).json()


def main():
    if HUE_USER == None:
        raise Exception("`HUE_USER` is not set")
    if HUE_ADDRESS == None:
        raise Exception("`HUE_ADDRESS` is not set")

    parser = argparse.ArgumentParser()
    parser.add_argument("light")
    args = parser.parse_args()

    lights = get_lights()

    if args.light == "list":
        for id, light in lights.items():
            print(id, light)
    else:
        idx = LIGHT_MAP[args.light]
        state = lights[idx][1]
        switch_light(idx, not state)


if __name__ == "__main__":
    main()
