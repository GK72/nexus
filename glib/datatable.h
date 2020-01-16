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

    std::string_view at(gint recIdx, const std::string& field) const;

    void display();
    void read();

    template <class Condition = std::equal_to<>>
    TableView filterAndSelect(const std::string& target, const std::string& criteria
                             ,const std::vector<std::string>& selection
                             ,Condition condition = Condition());

    template <class Filter>
    TableView& filterRecords(TableView& view, Filter filter, const std::string& by);

    TableView& selectFields(TableView& view, const std::vector<std::string>& selection);

    void update(const TableView& view, const std::string& value, const std::string& field = "");

private:
    std::string name;
    IO::ParserCSV* m_reader;
    // TODO: use std::any instead of std::string
    std::map<std::string, IO::ParserCSV::record> m_data;

    gint m_nRow = 0;

};


struct TableView {
    TableView(DataTable* pData);

    DataTable* data = nullptr;
    std::vector<gint> records;
    std::vector<std::string> fields;

    void display() const;
    DataTable materialize();

private:
    void displayHeader() const;
    void displayView() const;
};



} // End of namespace glib