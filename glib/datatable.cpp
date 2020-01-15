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

void DataTable::display()
{
    gint maxWidth = 100;
    gint currentWidth = 0;

    //TableView view = filterAndSelect("Date", "2014.01.07", { "Price" }, std::less<>());
    TableView view = filterAndSelect("Date", "2014.01.07", { "Price" });

    for (const auto& header : view.fields) {
        std::cout << header << ' ';
    }
    std::cout << '\n';

    for (const auto& recIdx : view.records) {
        for (const auto& fieldIdx : view.fields) {
            auto rec = m_records.at(recIdx);
            auto field = rec.at(m_header.at(fieldIdx));
            std::cout << field << ' ';
        }
        std::cout << '\n';
    }
}

void DataTable::read()
{
    m_header = m_reader->readHeader();
    IO::ParserCSV::record value;
    while ((value = m_reader->readRecord()).size() > 0) {
        m_records.push_back(value);
    }
}

template <class Condition>
TableView DataTable::filterAndSelect(const std::string& target, const std::string& criteria
                                    ,const std::vector<std::string>& selection
                                    ,Condition condition)
{
    return filterRecords(
        selectFields(TableView(), selection)
        , [&](auto x) -> bool {
            return condition(
                x.at(m_header.at(target))
                ,criteria);
        }
    );
}

template <class Filter>
TableView& DataTable::filterRecords(TableView& view, Filter filter)
{
    if (!view.data) {
        view.data = this;
    }
    else if (view.data != this) {
        throw std::runtime_error("View refers to another DataTable");
    }

    for (gint i = 0; i < m_records.size(); ++i) {
        if (filter(m_records[i])) {
            view.records.push_back(i);
        }
    }

    return view;
}

TableView& DataTable::selectFields(TableView& view, const std::vector<std::string>& selection)
{
    if (!view.data) {
        view.data = this;
    }
    else {
        throw std::runtime_error("View refers to another DataTable");
    }

    for (const auto& header : m_header) {
        for (const auto& s : selection) {
            if (header.first == s) {
                view.fields.push_back(header.first);
            }
        }
    }

    return view;
}



} // End of namespace glib