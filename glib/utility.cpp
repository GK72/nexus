// **********************************************
// ** gkpro @ 2019-08-29                       **
// ** G-Library                                **
// **   Utility library implementation         **
// **                                          **
// **********************************************

#include <cstdint>
#include "utility.h"
#include "utility_t.hpp"
#include "gmath.h"

namespace glib {


template <class T> iterator<T>::iterator()                          { p = nullptr; }
template <class T> iterator<T>::iterator(T* p)                      : p(p) {}
template <class T> bool iterator<T>::operator!=(iterator rhs)       { return p != rhs.p; }
template <class T> T& iterator<T>::operator*()                      { return *p; }
template <class T> iterator<T>& iterator<T>::operator++()           { ++p; return *this; }
template <class T> iterator<T> iterator<T>::operator++(int)
{
    iterator<T> t(p);
    ++(*p);
    return t;
}

index::index(const std::vector<size_t>& dim) : dims(dim) {}

size_t index::at(const std::vector<size_t>& vec) const {
    _global = vec[vec.size() - 1];
    size_t dm = 1;
    for (size_t i = vec.size(); i > 1; --i) {
        dm *= dims[i-1];
        _global = vec[i-2] * dm + _global;
    }
    return _global;
}

size_t index::at(size_t x, size_t y) const { return x * dims[1] + y; }


} //End of namespace glib