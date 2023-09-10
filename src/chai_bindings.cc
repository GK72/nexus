#include "chai_bindings.h"

#include <string>

namespace {

std::string welcome(const std::string& name) {
    return "Welcome " + name;
}

} // namespace

namespace nxs {

chai::chai() {
    m_chai.add(chaiscript::fun(&welcome), "welcome");
}

void chai::eval_file(const std::string& filepath) {
    m_chai.eval_file(filepath);
}

chaiscript::Boxed_Value chai::eval(const std::string& script) {
    return m_chai.eval(script);
}

} // namespace nxs
