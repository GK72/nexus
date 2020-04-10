// **********************************************
// ** gkpro @ 2019-12-23                       **
// **                                          **
// **           ---  G-Library  ---            **
// **        Big Integer implentation          **
// **                                          **
// **********************************************

#include "bigint.h"

namespace glib {

std::ostream& operator<<(std::ostream& out, const BigInt& rhs) {
    out << rhs.toString();
}

BigInt& BigInt::operator++() {
    // TODO: increment
    return *this;
}

BigInt& BigInt::operator++(int) {
    auto tmp(*this);
    operator++();
    return tmp;
}

BigInt& BigInt::operator+=(const BigInt& rhs) {
    size_t size = noDigits();
    if (noDigits() < rhs.noDigits()) {
        size = rhs.noDigits();
    }

    if (m_isNegative != rhs.m_isNegative) {
        return *this -= rhs;
    }

    char carry = 0;
    for (size_t i = 0; i < size; ++i) {
        char sum = getDigit(i) + rhs.getDigit(i);
        carry    = sum / BASE;
        char sum = sum % BASE;
        setDigit(i, sum);
    }

    if (carry) {
        addMSD(carry);
    }

    return *this;
}

BigInt::BigInt() {
    m_digit.push_back(0);
}


std::string BigInt::toString() const {
    std::string str;
    if (m_isNegative) {
        str += '-';
    }

    for (size_t i = noDigits() - 1; i >= 0; --i) {
        str += getDigit(i);
    }
    return str;
}

void BigInt::print() const {
    std::cout << toString();
}



} // End of namespace glib