#pragma once

#include <exception>
#include <utility>
#include <vector>

#include "utility.h"

namespace nxs {

class OutOfRange : public std::exception {
public:
    OutOfRange(size_t i, size_t j, size_t n, size_t m)
        : msg(joinStr("", "Index ", i, ",", j, " out of range (", n - 1, ",", m - 1, ")"))
    {}
    const char* what() const noexcept override { return msg.c_str(); }

private:
    std::string msg;
};

template <class T>
class Matrix {
public:
    Matrix(size_t n, size_t m);
    Matrix(size_t n, size_t m, const std::vector<T>& data);

    [[nodiscard]] constexpr T        operator()(size_t i, size_t j) const noexcept;
    [[nodiscard]] constexpr T&       operator()(size_t i, size_t j)       noexcept;
    [[nodiscard]] constexpr const T& operator[](size_t i)           const noexcept { return m_data[i]; }
    [[nodiscard]] constexpr T&       operator[](size_t i)                 noexcept { return m_data[i]; }
    [[nodiscard]] constexpr T at(size_t i, size_t j) const;

    [[nodiscard]] constexpr T      size() const noexcept { return m_data.size(); }
    [[nodiscard]] constexpr size_t rows() const noexcept { return m_rows; }
    [[nodiscard]] constexpr size_t cols() const noexcept { return m_cols; }

    template <class U> friend constexpr Matrix<U> operator+(const Matrix<U>& lhs, const Matrix<U>& rhs) noexcept;
    template <class U> friend constexpr Matrix<U> operator*(const Matrix<U>& lhs, const Matrix<U>& rhs) noexcept;
    template <class U> friend constexpr Matrix<U> operator*(U lhs               , const Matrix<U>& rhs) noexcept;
    template <class U> friend constexpr Matrix<U> operator*(const Matrix<U>& lhs,                U rhs) noexcept;
    template <class U> friend constexpr bool operator==(const Matrix<U>& lhs, const Matrix<U>& rhs) noexcept;

    [[nodiscard]] constexpr Matrix<T> Tr() const noexcept;


    // auto det();

private:
    size_t m_rows;
    size_t m_cols;
    std::vector<T> m_data;
};


template <class T>
constexpr Matrix<T> operator+(const Matrix<T>& lhs, const Matrix<T>& rhs) noexcept {
    auto ret = Matrix<T>(lhs.rows(), lhs.cols());

    for (size_t i = 0; i < lhs.size(); ++i) {
        ret[i] = lhs[i] + rhs[i];
    }

    return ret;
}

template <class T>
constexpr Matrix<T> operator*(const Matrix<T>& lhs, const Matrix<T>& rhs) noexcept {
    auto ret = Matrix<T>(lhs.rows(), rhs.cols());

    for (size_t i = 0; i < lhs.m_rows; ++i) {
        for (size_t j = 0; j < rhs.m_cols; ++j) {
            ret(i, j) = T(0);
            for (size_t k = 0; k < lhs.m_cols; ++k) {
                ret(i, j) += lhs.at(i, k) * rhs.at(k, j);
            }
        }
    }

    return ret;
}

template <class T>
constexpr Matrix<T> operator*(T lhs, const Matrix<T>& rhs) noexcept {
    auto ret = Matrix<T>(rhs.rows(), rhs.cols());

    for (size_t i = 0; i < rhs.size(); ++i) {
        ret[i] = lhs * rhs[i];
    }

    return ret;
}

template <class U>
constexpr Matrix<U> operator*(const Matrix<U>& lhs, U rhs) noexcept {
    return rhs * lhs;
}

template <class R1, class R2>
class Zip {
public:
    Zip(const R1& r1, const R2& r2) : m_r1(r1), m_r2(r2) {}

    auto begin() const      { return std::make_pair(std::begin(m_r1), std::begin(m_r2)); }
    auto end() const        { return std::make_pair(std::end(m_r1)  , std::end(m_r2));   }

    auto operator++()       { return std::make_pair(++m_r1, ++m_r2); }
    auto operator++(int)    { return std::make_pair(m_r1++, m_r2++); }
    auto& operator*()       { return std::make_pair(*m_r1, *m_r2); }

private:
    const R1& m_r1;
    const R2& m_r2;
};

template <class T>
constexpr bool operator==(const Matrix<T>& lhs, const Matrix<T>& rhs) noexcept {
    auto itL = std::cbegin(lhs.m_data);
    auto itR = std::cbegin(rhs.m_data);

    for (; itL != std::end(lhs.m_data); ++itL, ++itR) {
        if (*itL != *itR) {
            return false;
        }
    }

    // for (const auto& [itL, itR] : Zip(lhs.m_data, rhs.m_data)) {
        // if (*itL != *itR) {
            // return false;
        // }
    // }

    return true;
}

template <class T>
constexpr bool operator!=(const Matrix<T>& lhs, const Matrix<T>& rhs) noexcept {
    return !(lhs == rhs);
}

template <class T>
Matrix<T>::Matrix(size_t n, size_t m)
    : m_rows(n)
    , m_cols(m)
    , m_data(n * m)
{}

template <class T>
Matrix<T>::Matrix(size_t n, size_t m, const std::vector<T>& data)
    : Matrix(n, m)
{
    m_data = data;
}

template <class T>
[[nodiscard]] constexpr
T Matrix<T>::operator()(size_t i, size_t j) const noexcept {
    return m_data[i * m_cols + j];
}

template <class T>
[[nodiscard]] constexpr
T& Matrix<T>::operator()(size_t i, size_t j) noexcept {
    return m_data[i * m_cols + j];
}

template <class T>
[[nodiscard]] constexpr
T Matrix<T>::at(size_t i, size_t j) const {
    if (i < m_rows && j < m_cols) {
        return m_data[i * m_cols + j];
    }
    throw OutOfRange(i, j, m_rows, m_cols);
}

template <class T>
constexpr Matrix<T> Matrix<T>::Tr() const noexcept {
    auto ret = Matrix<T>(m_cols, m_rows);

    for (size_t i = 0; i < m_rows; ++i) {
        for (size_t j = 0; j < m_cols; ++j) {
            ret(j,i) = at(i,j);
        }
    }

    return ret;
}

// template <class T>
// [[nodiscard]] constexpr
// auto Matrix<T>::det() {
    //  2 1 3
    //  2 0 1
    // -1 1 2
    //
    //  2 * (0 * 2 - 1 * 1)  =  2 * -1      = -2
    // -1 * (2 * 2 - 1 * -1) = -1 *  4 - -1 = -5
    //  3 * (2 * 1 - 0 * -1) =  3 *  2 - 0  =  6
    //
    // ad - bc
    // at(i, j) * det()
// }

} // namespace nxs
