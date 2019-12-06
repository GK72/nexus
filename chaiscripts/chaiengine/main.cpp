#include <chaiscript/chaiscript.hpp>
#include <string>
#include "functions.h"

struct Chai {
    chaiscript::ChaiScript chai;

    void runscript(const std::string& filepath) {
        chai.eval_file(filepath);
    }
};


int main(int argc, char* args[]) {
    auto fileScript = "script.chai";
    if (argc == 2) {
        fileScript = args[1];
    }

    //chaiscript::ChaiScript chai;
    Chai chai;

    chai.chai.add(chaiscript::fun(&welcome), "welcome");
    chai.chai.add(chaiscript::fun(&inc), "increment");
    chai.chai.add(chaiscript::fun(&Chai::runscript, &chai), "runscript");
    chai.chai.eval_file(fileScript);
}
