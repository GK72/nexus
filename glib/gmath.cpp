// **********************************************
// ** gkpro @ 2019-09-04                       **
// **                                          **
// **           ---  G-Library  ---            **
// **       Math library implementation        **
// **              Class: Matrix               **
// **********************************************

#include <cstring>
#include <sstream>

#include "gmath.h"
#include "gmath_t.hpp"

namespace glib {

template <class T> using iType = iterator<T>;

template <class T> std::ostream& operator<< (std::ostream& out, const Matrix<T>& rhs) {
    for (size_t i = 0; i < rhs.n; ++i) {
        for (size_t j = 0; j < rhs.m; ++j) {
            out << rhs.at(i, j) << rhs.ws;
        }
        out << '\n';
    }
    return out;
}

template <class T> std::istream& operator>> (std::istream& in, Matrix<T>& rhs) {
    for (size_t i = 0; i < rhs.length; ++i) {
        in >> rhs.elem[i];
    }
    return in;
}

template <class T> Matrix<T> operator+ (const T& lhs, const Matrix<T>& rhs) {
    Matrix<T> res(rhs.n, rhs.m);
    for (size_t i = 0; i < rhs.length; ++i) {
        res.elem[i] = lhs + rhs.elem[i];
    }
    return res;
}

template <class T> Matrix<T> operator+ (const Matrix<T>& lhs, const T& rhs) {
    return Matrix<T>(rhs + lhs);
}

template <class T> Matrix<T> operator+ (const Matrix<T>& lhs, const Matrix<T>& rhs) {
    if (lhs.n == rhs.n && lhs.m == rhs.m) {
        Matrix<T> res(lhs.n, rhs.m);
        for (size_t i = 0; i < lhs.length; ++i) {
            res[i] = lhs[i] + rhs[i];
        }
        return lhs;
    }
    else throw error_unconformable;
}

template <class T> Matrix<T> operator- (const T& lhs, const Matrix<T>& rhs) {
    Matrix<T> res(rhs.n, rhs.m);
    for (size_t i = 0; i < rhs.length; ++i) {
        res.elem[i] = lhs - rhs.elem[i];
    }
    return res;
}

template <class T> Matrix<T> operator- (const Matrix<T>& lhs, const T& rhs) {
    return Matrix<T>(rhs - lhs);
}

template <class T> Matrix<T> operator- (const Matrix<T>& lhs, const Matrix<T>& rhs) {
    if (lhs.n == rhs.n && lhs.m == rhs.m) {
        Matrix<T> res(lhs.n, rhs.m);
        for (size_t i = 0; i < lhs.length; ++i) {
            res[i] = lhs[i] - rhs[i];
        }
        return lhs;
    }
    else throw error_unconformable;
}

template <class T> Matrix<T> operator* (const T& lhs, const Matrix<T>& rhs) {
    Matrix<T> res(rhs.n, rhs.m);
    for (size_t i = 0; i < rhs.length; ++i) {
        res.elem[i] = lhs * rhs.elem[i];
    }
    return res;
}

template <class T> Matrix<T> operator* (const Matrix<T>& lhs, const T& rhs) {
    return Matrix<T>(rhs * lhs);
}

template <class T> Matrix<T> operator* (const Matrix<T>& lhs, const Matrix<T>& rhs) {
    if (lhs.m == rhs.n) {
        T sum;
        Matrix<T> res(lhs.n, rhs.m);
        for (size_t i = 0; i < res.n; ++i) {
            for (size_t j = 0; j < res.m; ++j) {
                sum = 0;
                for (size_t k = 0; k < lhs.m; ++k) {
                    sum += lhs.at(i, k) * rhs.at(k, j);
                }
                res.at(i, j) = sum;
            }
        }
        return res;
    }
    else throw error_unconformable;
}

template <class T> Matrix<T> epow(const Matrix<T>& lhs, double exp) {
    Matrix<T> res(lhs.n, lhs.m);
    for (size_t i = 0; i < lhs.length; ++i) {
        res.elem[i] = std::pow(lhs.elem[i], exp);
    }
    return res;
}

template <class T> Matrix<T> elog(const Matrix<T>& lhs) {
    Matrix<T> res(lhs.n, lhs.m);
    for (size_t i = 0; i < lhs.length; ++i) {
        res.elem[i] = std::log10(lhs.elem[i]);
    }
    return res;
}
template <class T> Matrix<T> edot(const Matrix<T>& lhs, const Matrix<T>& rhs) {
    if (lhs.n == rhs.n && lhs.m == rhs.m) {
        Matrix<T> res(lhs.n, lhs.m);
        for (size_t i = 0; i < lhs.length; ++i) {
            res.elem[i] = lhs.elem[i] * rhs.elem[i];
        }
        return res;
    }
    else throw error_unconformable;
}

// ************************************************************************** //

template <class T> Matrix<T>::Matrix()                                  : Matrix(1, 1) {}
template <class T> Matrix<T>::Matrix(gint n)                            : Matrix(n, 1) {}
template <class T> Matrix<T>::Matrix(gint n, gint m)                    : idx(pDim{n, m}) { init(n, m); }
template <class T> Matrix<T>::~Matrix()                                 { delete[] elem; }
template <class T> Matrix<T>::Matrix(const Matrix &rhs) :
    idx(pDim{rhs.n, rhs.m})
{
    resize(rhs.n, rhs.m);
    for (size_t i = 0; i < length; ++i) {
        elem[i] = rhs.elem[i];
    }
}

template <class T> Matrix<T>::Matrix(Matrix&& rhs) noexcept :
    elem(std::move(rhs.elem)),
    n(rhs.n), m(rhs.m),
    idx(pDim{n, m})
{
    rhs.elem = nullptr;
}

template <class T> Matrix<T>& Matrix<T>::operator=(const Matrix& rhs) {
    if (this == &rhs)
        return *this;

    delete[] elem;
    n = rhs.n;
    m = rhs.m;
    length = n * m;

    elem = new T[length];
    for (size_t i = 0; i < length; ++i) {
        elem[i] = rhs.elem[i];
    }
    return *this;
}

template <class T> Matrix<T>& Matrix<T>::operator=(Matrix &&rhs) noexcept {
    if (this == &rhs)
        return *this;

    elem = std::move(rhs.elem);
    n = rhs.n;
    m = rhs.m;
    rhs.elem = nullptr;
    return *this;
}

template<class T> void Matrix<T>::importIDX(const char* filename) {
    // First 4 bytes are the magic number
    //   - The 1st and 2nd bytes are always 0
    //   - The 3rd byte codes the type of the data: see bitmasks
    //   - The 4th byte codes the number of dimensions of the vector/matrix: 1 for vectors, 2 for matrices
    // The integers are 32 bit and in high endian format - MSB first
    
    const unsigned int bms_ubyte  = 0x0000'0800;          // unsigned byte 
    const unsigned int bms_byte   = 0x0000'0900;          // signed byte 
    const unsigned int bms_short  = 0x0000'0B00;          // short (2 bytes) 
    const unsigned int bms_int    = 0x0000'0C00;          // int (4 bytes) 
    const unsigned int bms_flot   = 0x0000'0D00;          // float (4 bytes) 
    const unsigned int bms_double = 0x0000'0E00;          // double (8 bytes)
    
    // Opening file in binary mode

    std::ifstream inf;
    inf.open(filename, std::ios::binary);
    if (!inf.is_open()) throw IOErrorException(filename);

    uint32_t magic = 0;
    uint32_t items = 0;
    char datasize = 1;
    
    // Reading header

    inf.read(reinterpret_cast<char*>(&magic), sizeof magic);    swapEndian32(magic);
    inf.read(reinterpret_cast<char*>(&items), sizeof items);    swapEndian32(items);
    unsigned char dims = magic & 0x000000FF;
    unsigned int bms_datatype = magic & bms_ubyte;
    
    // Dimensions of the matrix

    uint32_t dim_n = items;
    uint32_t dim_m = 1;
    uint32_t dim_tmp = 1;

    for (unsigned char i = 1; i < dims; ++i) {
        inf.read(reinterpret_cast<char*>(&dim_tmp), sizeof dim_tmp);
        dim_m *= swapEndian32(dim_tmp);
    }

    // Resizing matrix
    resize(dim_n, dim_m);

    // Reading data

    unsigned char byte;
    size_t i = 0;

    while (inf.read(reinterpret_cast<char*>(&byte), datasize) && i < items) {
        elem[i] = byte;
        ++i;
    }
}

template<class T> void Matrix<T>::exportTXT(const char* file) {
    std::ofstream outf(file, std::ios::out);
    std::stringstream ss;
    std::ios::sync_with_stdio(false);
    for (size_t i = 0; i < n; ++i) {
        outf << getRow(i);
    }
    outf.close();
    std::ios::sync_with_stdio(true);
}

template<class T> void Matrix<T>::init(gint _n, gint _m) {
    n = _n;
    m = _m;
    length = n * m;
    elem = new T[length];
}

template<class T> void Matrix<T>::resize(gint n, gint m) {
    delete[] elem;
    init(n, m);
}

// Transpose
template <class T> Matrix<T> Matrix<T>::tr() {
    Matrix<T> res(m, n);
    for (size_t i = 0; i < res.n; ++i) {
        for (size_t j = 0; j < res.m; ++j) {
            res.at(i, j) = this->at(j, i);
        }
    }
    return res;
}

template <class T> void Matrix<T>::set(const Matrix<T>& in) {
    if (in.n == n && in.m == m) {
        for (size_t i = 0; i < length; ++i) {
            elem[i] = in.elem[i];
        }
    }
}

// Methods for Range based loops

template <class T> iType<T> Matrix<T>::begin()                      { return iType(elem); }
template <class T> iType<T> Matrix<T>::end()                        { return iType(elem + length); }

// Getter methods

template <class T> T  Matrix<T>::at(pDim d) const                   { return elem[idx.at(d)]; }
template <class T> T& Matrix<T>::at(pDim d)                         { return elem[idx.at(d)]; }
template <class T> T  Matrix<T>::at(gint x, gint y) const           { return elem[x * m + y]; }
template <class T> T& Matrix<T>::at(gint x, gint y)                 { return elem[x * m + y]; }
template <class T> const T& Matrix<T>::operator[] (gint i) const    { return elem[i]; }
template <class T>       T& Matrix<T>::operator[] (gint i)          { return elem[i]; }
template <class T> gint Matrix<T>::getLength()                      { return length; }
template <class T> gint Matrix<T>::getN()                           { return n; }
template <class T> gint Matrix<T>::getM()                           { return m; }

template <class T> Matrix<T> Matrix<T>::getRow(gint r) {
    Matrix<T> row(1, m);
    for (size_t i = 0; i < m; ++i) {
        row.elem[i] = elem[idx.at(r, i)];
    }
    //memcpy(&row.elem, &elem[idx.at(r, 0)], sizeof row.elem[0] * m);
    return row;
}

// ************************************************************************** //
//                                Functions                                   //
// ************************************************************************** //

inline double cost(double x, double y)      { return std::pow(x - y, 2); }
inline double sigmoid(double x)             { return 1 / (1 + std::pow(e, -x)); }
inline double estDerivative(double x)       { return (x + epsilon - (x - epsilon))
                                                   / (2 * epsilon); }

template <class T> Matrix<T> sigmoid(Matrix<T>& x) {
    gint length = x.getLength();
    Matrix<T> res(x.getN(), x.getM());
    for (size_t i = 0; i < length; i++)    {
        res[i] = sigmoid(x[i]);
    }
    return res;
}



} // End of namespace glib