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

} // namespace nxs
