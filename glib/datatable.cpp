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

std::string_view DataTable::at(gint recIdx, const std::string& field) const
{
    return m_data.at(field)[recIdx];
}

void DataTable::display()
{
    gint maxWidth = 100;
    gint currentWidth = 0;

    TableView view = filterAndSelect("Date", "2014.01.07", { "Date", "Price" } /* , std::less<>() */);
    view.display();
}

void DataTable::read()
{
    std::map<std::string, gint> header = m_reader->readHeader();
    IO::ParserCSV::record value;
    while ((value = m_reader->readRecord()).size() > 0) {
        for (const auto& h : header) {
            m_data[h.first].push_back(value.at(h.second));
        }
    }
    m_nRow = m_data.begin()->second.size();
}

template <class Condition>
TableView DataTable::filterAndSelect(const std::string& target, const std::string& criteria
                                    ,const std::vector<std::string>& selection
                                    ,Condition condition)
{
    return filterRecords(
        selectFields(TableView(this), selection)
        , [&](auto x) -> bool { return condition(x, criteria); }
        , target
    );
}

template <class Filter>
TableView& DataTable::filterRecords(TableView& view, Filter filter, const std::string& by)
{
    for (gint i = 0; i < m_nRow; ++i) {
        if (filter(m_data.at(by)[i])) {
            view.records.push_back(i);
        }
    }
    return view;
}

TableView& DataTable::selectFields(TableView& view, const std::vector<std::string>& selection)
{
    for (const auto& header : m_data) {
        for (const auto& s : selection) {
            if (header.first == s) {
                view.fields.push_back(header.first);
            }
        }
    }
    return view;
}

TableView::TableView(DataTable* pData) : data(pData)
{

}

void TableView::display() const
{
    displayHeader();
    displayView();
}

void TableView::displayHeader() const
{
    for (const auto& header : fields) {
        std::cout << header << ' ';
    }
    std::cout << '\n';
}

void TableView::displayView() const
{
    for (const auto& recIdx : records) {
        for (const auto& fieldIdx : fields) {
            std::cout << data->at(recIdx, fieldIdx) << ' ';
        }
        std::cout << '\n';
    }
}

DataTable TableView::materialize()
{
    // TODO: creating a new DataTable from the current view
    _g_NIE("TableView::materialize");
    return DataTable("");
}



} // End of namespace glib