// **********************************************
// ** gkpro @ 2020-01-13                       **
// **                                          **
// **      Datatable testing application       **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include <iostream>
#include <string_view>
#include "datatable.h"

void datatable_test(std::string_view path)
{
    std::cout << "Datatable testing... (path: " << path << ")\n";
    glib::DataTable data(path);
    data.read();
    data.print();
}