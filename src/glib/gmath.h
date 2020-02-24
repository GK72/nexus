// **********************************************
// ** gkpro @ 2019-09-04                       **
// **                                          **
// **           ---  G-Library  ---            **
// **           Math library header            **
// **              Class: Matrix               **
// **********************************************

#pragma once
#include <cmath>
#include <exception>
#include <fstream>
#include <string>
#include "utility.h"

namespace glib {

using gint = size_t;

struct Point2D;
class Line2D;

constexpr const char* error_unconformable = "Unconformable matrices";
constexpr double e = 2.71828182845904523536;                // Euler's constant
constexpr double epsilon = 1E-04;

// ************************************************************************** //
//                               Exceptions                                   //
// ************************************************************************** //



// ************************************************************************** //
//                              Matrix class                                  //
// ************************************************************************** //

template <class T> class Matrix;

template <class T> std::ostream& operator<< (std::ostream& out, const Matrix<T>& rhs);
template <class T> std::istream& operator>> (std::istream& in, Matrix<T>& rhs);
template <class T> Matrix<T> operator+ (const T& lhs, const Matrix<T>& rhs);
template <class T> Matrix<T> operator+ (const Matrix<T>& lhs, const T& rhs);
template <class T> Matrix<T> operator+ (const Matrix<T>& lhs, const Matrix<T>& rhs);
template <class T> Matrix<T> operator- (const T& lhs, const Matrix<T>& rhs);
template <class T> Matrix<T> operator- (const Matrix<T>& lhs, const T& rhs);
template <class T> Matrix<T> operator- (const Matrix<T>& lhs, const Matrix<T>& rhs);
template <class T> Matrix<T> operator* (const T& lhs, const Matrix<T>& rhs);
template <class T> Matrix<T> operator* (const Matrix<T>& lhs, const T& rhs);
template <class T> Matrix<T> operator* (const Matrix<T>& lhs, const Matrix<T>& rhs);
template <class T> Matrix<T> epow(const Matrix<T>& lhs, double exp);
template <class T> Matrix<T> elog(const Matrix<T>& lhs);
template <class T> Matrix<T> edot(const Matrix<T>& lhs, const Matrix<T>& rhs);

// ************************************************************************** //

template <class T> class Matrix {
    using iType = Iterator<T>;                          // Iterator type

public:
    Matrix();
    Matrix(gint n);
    Matrix(gint n, gint m);
    ~Matrix();
    Matrix(const Matrix &rhs);
    Matrix(Matrix&& rhs) noexcept;
    Matrix& operator=(const Matrix& rhs);
    Matrix& operator=(Matrix &&rhs) noexcept;

    friend std::ostream& operator<< <>(std::ostream& out, const Matrix<T>& rhs);
    friend std::istream& operator>> <>(std::istream& in, Matrix<T>& rhs);

    friend Matrix operator+ <>(const T& lhs, const Matrix<T>& rhs);
    friend Matrix operator+ <>(const Matrix<T>& lhs, const T& rhs);
    friend Matrix operator+ <>(const Matrix<T> &lhs, const Matrix<T> &rhs);
    friend Matrix operator- <>(const T& lhs, const Matrix<T>& rhs);
    friend Matrix operator- <>(const Matrix<T>& lhs, const T& rhs);
    friend Matrix operator- <>(const Matrix<T> &lhs, const Matrix<T> &rhs);
    friend Matrix operator* <>(const T& lhs, const Matrix<T>& rhs);
    friend Matrix operator* <>(const Matrix<T>& lhs, const T& rhs);
    friend Matrix operator* <>(const Matrix<T>& lhs, const Matrix<T>& rhs);

    friend Matrix epow <>(const Matrix<T>& lhs, double exp);
    friend Matrix elog <>(const Matrix<T>& lhs);
    friend Matrix edot <>(const Matrix<T>& lhs, const Matrix<T>& rhs);

    // IO functions

    void importIDX(const char* file);
    void exportTXT(const char* file);

    void init(gint n, gint m);
    void resize(gint n, gint m);

    // Transpose
    Matrix<T> tr();

    void set(const Matrix<T>& in);

    // Methods for Range based loops

    iType begin();
    iType end();

    // Getter methods

    T  at(pDim d) const;
    T& at(pDim d);
    T  at(gint x, gint y) const;
    T& at(gint x, gint y);
    const T& operator[] (gint i) const;
          T& operator[] (gint i);
    
    gint getLength();
    gint getN();
    gint getM();

    Matrix<T> getRow(gint r);

private:
    T* elem;
    gint n;                                 // Rows
    gint m;                                 // Columns
    gint length = n * m;
    
    iType it;
    Index idx;

    char ws = ' ';                          // Whitespace

    // Private functions

};

// ************************************************************************** //
//                                Functions                                   //
// ************************************************************************** //

inline double cost(double x, double y);
inline double sigmoid(double x);
inline double estDerivative(double x);
template <class T> Matrix<T> sigmoid(Matrix<T>& x);



} // End of namespace glib