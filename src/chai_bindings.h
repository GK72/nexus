#pragma once

#include <chaiscript/chaiscript.hpp>

namespace nxs {

class chai {
public:
    chai();

    void eval_file(const std::string& filepath);

private:
    chaiscript::ChaiScript m_chai;

};

} // namespace nxs
