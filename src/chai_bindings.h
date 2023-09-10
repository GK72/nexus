#pragma once

#include <chaiscript/chaiscript.hpp>

namespace nxs {

class chai {
public:
    chai();

    void eval_file(const std::string& filepath);
    chaiscript::Boxed_Value eval(const std::string& script);

    template <typename T>
    void add_ref(T& obj, const std::string& name) {
        m_chai.add(chaiscript::var(std::ref(obj)), name);
    }

    chaiscript::ChaiScript& engine() { return m_chai; }

private:
    chaiscript::ChaiScript m_chai;

};

} // namespace nxs
