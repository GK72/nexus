// **********************************************
// ** gkpro @ 2019-09-22                       **
// **                                          **
// **     Console application entry point      **
// **              ---  RPG  ---               **
// **                                          **
// **                                          **
// **********************************************

#include "rpg.h"

std::mutex mx;

namespace glib {
namespace rpg {


Game::Game(gint width, gint height, gint fontWidth, gint fontHeight)
{
#ifdef _MSC_BUILD
    gfx = new glib::gGfx::EngineConW(this, (int)width, (int)height, (int)fontWidth, (int)fontHeight);
#else
    gfx = new glib::gGfx::EngineCurses(150, 40, 7, 14);
#endif
}

Game::~Game()
{
    delete hero;
    delete gfx;
    if (frame) delete frame;
}

void Game::run()
{
    gfx->run();
}

void Game::init()
{
    mt = std::mt19937(rd());
    hero = new Hero(this, "Aida");
    glib::gGfx::FrameBuilder framebuilder(gfx);
    frames["Main"] = framebuilder.createFrame("Main", Point2D(32, 8));
    frames["Map"] = framebuilder.createFrame("Map", Point2D(32, 16), Point2D(50, 0));

    new glib::gGfx::FrameContentText(frames["Main"], hero->toString() + "\nAttack: ");
}

int Game::inputHandling()
{
    inputEventNo = 0;
    if (gfx->getKey(VK_SPACE).isPressed) {
        hero->attack();
        ++inputEventNo;
    }

    return inputEventNo;
}

void Game::update(float elapsedTime)
{
    // Fetch event message and dispatch it

    for (const auto& e : frames) {
        e.second->draw();
    }
}

void Game::trigger(Event& evt)
{
    frames["Main"]->setContent(hero->toString() + "\nAttack: " + evt.msg);
}



Hero::Hero(Game* engine, std::string name)
{
    _engine = engine;
    _name = name;
    _heroLevel = 1;
    _experience = 0;

    _damageMin = 1;
    _damageMax = 5;

    attach(engine);
}

std::string Hero::toString() const
{
    return std::string(
        "Name : " + _name + '\n' +
        "Level: " + std::to_string(_heroLevel) + '\n' +
        "Exp  : " + std::to_string(_experience)
    );
}

void Hero::move(glib::gGfx::DIRECTION_2D direction)
{

}

int Hero::attack()
{
    std::uniform_int_distribution<> dist(_damageMin, _damageMax);
    int dmg = dist(_engine->rng());
    notify(Event(std::to_string(dmg)));
    return dmg;
}



} // End of namespace rpg
} // End of namespace glib