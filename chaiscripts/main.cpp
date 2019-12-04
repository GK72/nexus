#include <chaiscript/chaiscript.hpp>
#include "functions.h"

int main(int argc, char* args[]) {
    auto fileScript = "script.chai";
    if (argc == 2) {
        fileScript = args[1];
    }

    chaiscript::ChaiScript chai;

    chai.add(chaiscript::fun(&welcome), "welcome");
    chai.add(chaiscript::fun(&inc), "increment");
    chai.eval_file(fileScript);
}
