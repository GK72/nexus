#pragma once
#include "gGfx.h"

namespace glib {
namespace rpg {

class EngineGTT : public glib::gGfx::EngineConW
{
public:
    EngineGTT();

    void init() override;
    int  input() override;
    void update(float elapsedTime) override;

private:
    std::string outstr;
};



} // End of namespace rpg
} // End of namespace glib