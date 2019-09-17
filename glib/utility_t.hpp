// **********************************************
// ** gkpro @ 2019-09-04                       **
// **                                          **
// **           ---  G-Library  ---            **
// **             Utility library              **
// **        Template specializations          **
// **********************************************

template struct glib::iterator<unsigned char>;
template struct glib::iterator<char>;
template struct glib::iterator<int>;
template struct glib::iterator<double>;

template int glib::swapEndian32(int&);