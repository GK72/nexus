class CaesarCipher():
    def __init__(self, k: int):
        self._k = k

    def encode(self, msg):
        return [chr(ord(c) + self._k) for c in msg]

    def decode(self, msg):
        return [chr(ord(c) - self._k) for c in msg]


class VigenereCipher():
    def __init__(self, key: str):
        self._key = key

    def encode(self, msg: str):
        key = self._match_length(msg)
        return "".join(
            [self._shift_letter(m, k) for m, k in zip(msg.lower(), key.lower())]
        )

    def decode(self, msg):
        pass

    def _match_length(self, msg: str):
        x = len(msg) // len(self._key)
        x = x + 1 if len(msg) % len(self._key) > 0 else x
        return (self._key * x)[0:len(msg)]

    def _shift_letter(self, msg: str, key: str):
        m = ord(msg)
        k = ord(key)

        if m >= 65 and m <= 90:     # Uppercase
            m = m - 65
            k = k - 65
            return chr((m + k) % 26 + 65)
        if m >= 97 and m <= 122:    # Lowercase
            m = m - 97
            k = k - 97
            return chr((m + k) % 26 + 97)


if __name__ == "__main__":
    cipher = VigenereCipher("lemon")
    encoded = cipher.encode("attackatdawn")
    print("Encoded:", encoded)
    print("Decoded:", cipher.decode(encoded))

