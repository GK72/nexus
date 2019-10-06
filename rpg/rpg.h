#pragma once
#include "../gGfx/gGfx.h"
#include "../gGfx/frames.h"

namespace glib {
namespace rpg {

using glib::gGfx::Point2D;


class Hero {
public:
    Hero(std::string name);

    std::string toString() const;

private:
    std::string _name;
    gint _experience;
    gint _heroLevel;
};


class Game : public glib::gGfx::Engine {
public:
    Game(gint width, gint height, gint fontWidth, gint fontHeight);
    ~Game();
    void run();

protected:
    void init();
    int inputHandling();
    void update(float elapsedTime);

private:
    glib::gGfx::EngineGFX* gfx;
    std::string outstr;

    glib::gGfx::Frame* frame = nullptr;
    std::vector<glib::gGfx::Frame*> frames;
    
    Hero* hero = nullptr;
};



} // End of namespace rpg
} // End of namespace glib