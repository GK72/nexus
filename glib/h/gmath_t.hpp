// **********************************************
// ** gkpro @ 2019-08-04                       **
// **                                          **
// **           ---  G-Library  ---            **
// **              Math library                **
// **        Template specializations          **
// **********************************************

#pragma once

namespace glib {

template class Matrix<unsigned char>;
template class Matrix<char>;
template class Matrix<int>;
template class Matrix<double>;

template <class T> class Matrix;
template std::istream& operator>> <int>(std::istream&, Matrix<int>&);
template std::ostream& operator<< <int>(std::ostream&, const Matrix<int>&);



} // End of namespace glib
//template int glib::swapEndian32(int&);