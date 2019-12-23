// **********************************************
// ** gkpro @ 2019-12-23                       **
// **                                          **
// **           ---  G-Library  ---            **
// **            Big Integer header            **
// **                                          **
// **********************************************

#pragma once
#include <iostream>
#include <string>
#include <vector>

namespace glib {

using gint = size_t;

class BigInt
{
public:
    BigInt();
    BigInt(gint value);
    BigInt(std::string value);
    bool                 operator< (const BigInt& rhs);
    bool                 operator> (const BigInt& rhs);
    bool                 operator= (const BigInt& rhs);
    bool                 operator<=(const BigInt& rhs);
    bool                 operator>=(const BigInt& rhs);
    BigInt&              operator-=(const BigInt& rhs);
    BigInt&              operator+=(const BigInt& rhs);
    BigInt&              operator/=(const BigInt& rhs);
    BigInt&              operator*=(const BigInt& rhs);
    BigInt&              operator++();
    BigInt&              operator++(int);
    BigInt&              operator--();
    BigInt&              operator--(int);
    friend BigInt        operator- (const BigInt& lhs, const BigInt& rhs);
    friend BigInt        operator+ (const BigInt& lhs, const BigInt& rhs);
    friend BigInt        operator/ (const BigInt& lhs, const BigInt& rhs);
    friend BigInt        operator* (const BigInt& lhs, const BigInt& rhs);
    friend std::ostream& operator<<(std::ostream& out, const BigInt& rhs);
    friend std::istream& operator>>(std::istream& in , const BigInt& rhs);

    void print() const;
    std::string toString() const;

private:
    char BASE = 10;

    gint getDigit(gint k) const;
    gint noDigits() const;
    void setDigit(gint k, gint value);

    void addMSD(gint value);
    void trimLeadingZeroes();

    std::vector<char> m_digit;
    bool m_isNegative = false;
};



} // End of namespace glib