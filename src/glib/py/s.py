#!/usr/bin/sage

import sys
from sage.all import *

def char_range(c1, c2):
    """Generates the characters from `c1` to `c2`, inclusive."""
    for c in range(ord(c1), ord(c2)+1):
        yield chr(c)


def get_freq_dist(x, key):
    """ Get the value from frequency distibution table `x` by the given `key` """
    return frequency_distribution(x).function().get(AlphabeticStrings()(key), 0)


class Crypto:
    def __init__(self, text, key):
        self.s = AlphabeticStrings()
        self.text = self.s(text.upper().replace(" ", ""))
        self.key = self.s(key.upper().replace(" ", ""))

    def encrypt(self):
        self.cipher = VigenereCryptosystem(self.s, len(self.key))(self.key)
        return self.cipher(self.text)


class Crack():
    def __init__(self, enc_text, key_length):        
        self._text = enc_text
        self._key_length = key_length
        self._text_length = len(self._text)
        
        self._key_pos_freq = []
        self._text_freq = []
        
    def run(self):
        self._calc_freq(self._get_key_letters())
        self._align_freqs()
    
    def _get_key_letters(self):
        """ The returned list matches all letters in the encrypted
        text those were ciphered with the letters of the key
        """
        arr = []
        for i in range(self._text_length // self._key_length):
            arr.append(self._get_key_letter(i))
        return arr
    
    def _get_key_letter(self, letter_num):
        """ The returned letters match all letters in the
        encrypted text those were ciphered with a letter of the key
        """
        arr = []
        for i in range(self._text_length // self._key_length + 1):
            s = i * self._key_length + letter_num
            arr.append(self._text[s:s+1])
        return arr

    def _calc_freq(self, x):
        """ Calculates the frequency distributions of the letters
        in the encrypted text and in the matched letters"""
        for ch in char_range('A', 'Z'):
            self._key_pos_freq.append([])
            for i in range(len(x)):
                self._key_pos_freq[-1].append(get_freq_dist(x[i], ch))
            self._text_freq.append(get_freq_dist(self._text, ch))
            
    def _align_freqs(self):
        length = len(self._text_freq)
        sums = []
        maxes = []

        for key_letter in range(self._key_length):
            sums.append([])
            for shift in range(length):
                results = []
                for letter in range(length):
                    results.append(self._key_pos_freq[letter][key_letter] * self._text_freq[(letter + shift) % length])
                sums[-1].append((sum(results), shift))
            maxes.append(max(sums[-1], key=lambda x: x[0]))
        
        self._sums = sums
        self._maxes = maxes


key = "ab"
text = "Create a Vigenere cryptosystem of block length"

c = Crypto(text, key)
enc = c.encrypt()

crack = Crack(enc, len(key))
crack.run()