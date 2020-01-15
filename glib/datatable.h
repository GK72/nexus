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

struct TableView;


class DataTable {
public:
    DataTable(std::string_view path);
    ~DataTable();
    DataTable(const DataTable& rhs)             = delete;
    DataTable(DataTable&& rhs)                  = delete;
    DataTable& operator=(const DataTable& rhs)  = delete;
    DataTable& operator=(DataTable&& rhs)       = delete;

    void display();
    void read();

    template <class Condition = std::equal_to<>>
    TableView filterAndSelect(const std::string& target, const std::string& criteria
                             ,const std::vector<std::string>& selection
                             ,Condition condition = Condition());

    template <class Filter>
    TableView& filterRecords(TableView& view, Filter filter);

    TableView& selectFields(TableView& view, const std::vector<std::string>& selection);

private:
    std::string name;
    IO::ParserCSV* m_reader;
    std::map<std::string, gint> m_header;
    std::vector<IO::ParserCSV::record> m_records;

};


struct TableView {
    DataTable* data = nullptr;
    std::vector<gint> records;
    std::vector<std::string> fields;

};


TableView selectFields(const std::vector<std::string>& selection);



} // End of namespace glib