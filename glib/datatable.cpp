// **********************************************
// ** gkpro @ 2019-09-11                       **
// **                                          **
// **      Datatable testing application       **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include "datatable.h"

class DataTable
{
public:
    DataTable() {}
    ~DataTable() {}
    DataTable(const DataTable& rhs)				= delete;
    DataTable(DataTable&& rhs)					= delete;
    DataTable& operator=(const DataTable& rhs)	= delete;
    DataTable& operator=(DataTable&& rhs)		= delete;

    void readCSV(const std::string& fullpath, bool header = false, char delim = ',')
    {
        std::string str;
        std::string elem;
        std::stringstream ss;
        std::ifstream inf(fullpath);
        size_t col = 0;

        // Reading first line, populating map with vectors - number of columns
        // One column = one vector
        if (getline(inf, str)) {
            ss.clear();
            ss.str(str);

            while (getline(ss, elem, delim)) {
                if (header) {
                    record.insert({ elem, std::vector<std::any>() });
                }
                else {
                    record.insert({ std::to_string(col), std::vector<std::any>() });
                    record.at(std::to_string(col)).push_back(std::any(elem));
                    ++col;
                }
            }
        }

        // Reading the rest of the file
        while (getline(inf, str))
        {
            ss.clear();
            ss.str(str);
            col = 0;

            while (getline(ss, elem, delim)) {
                for (auto &col : record) {
                    col.second.push_back(std::any(elem));
                }
                //record.at(std::to_string(col)).push_back(std::any(elem));
                //++col;
            }
        }
        inf.close();
    }
    void print() {
        for (const auto &rec : record) {
            for (const auto &e : rec.second) {
                std::cout << std::any_cast<std::string>(e) << ' ';
            }
            std::cout << '\n';
        }
    }

private:
    std::string name;
    std::unordered_map<std::string,	std::vector<std::any>> record;
};