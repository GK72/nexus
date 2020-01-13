// **********************************************
// ** gkpro @ 2019-09-11                       **
// **                                          **
// **      Datatable testing application       **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#pragma once

#include <any>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

#include "io.h"

namespace glib {

using gint = size_t;

class DataTable {
public:
    DataTable(std::string_view path);
    ~DataTable();
    DataTable(const DataTable& rhs)             = delete;
    DataTable(DataTable&& rhs)                  = delete;
    DataTable& operator=(const DataTable& rhs)  = delete;
    DataTable& operator=(DataTable&& rhs)       = delete;

    void print();
    void read();

private:
    std::string name;
    IO::ParserCSV* m_reader;
    std::map<std::string, gint> m_header;
    std::vector<IO::ParserCSV::record> m_records;

};



} // End of namespace glib