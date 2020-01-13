// **********************************************
// ** gkpro @ 2019-09-11                       **
// **                                          **
// **      Datatable testing application       **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include "datatable.h"

namespace glib {


DataTable::DataTable(std::string_view path)
{
    m_reader = new IO::ParserCSV(path);
}

DataTable::~DataTable()
{
    delete m_reader;
}

void DataTable::print()
{
    for (const auto& rec : m_records) {
        std::cout << rec.at(m_header.at("Category")) << '\n';
    }

    //for (const auto &rec : record) {
    //    for (const auto &e : rec.second) {
    //        std::cout << std::any_cast<std::string>(e) << ' ';
    //    }
    //    std::cout << '\n';
    //}
}

void DataTable::read()
{
    m_header = m_reader->readHeader();
    int i = 0;
    IO::ParserCSV::record value;
    while ((value = m_reader->readRecord()).size() > 0 && i < 5) {
        m_records.push_back(value);
        ++i;
    }
    
}



} // End of namespace glib