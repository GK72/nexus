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


int Item::_uid = 0;

Game::Game(size_t width, size_t height, size_t fontWidth, size_t fontHeight)
{
#ifdef _MSC_BUILD
    gfx = new glib::gGfx::EngineConW(this, (int)width, (int)height, (int)fontWidth, (int)fontHeight);
#else
    gfx = new glib::gGfx::EngineCurses(150, 40, 7, 14);
#endif
}

Game::~Game()
{
    // Detach subscriber from publisher,
    // but in this case it isn't necessary
    // because the publisher gets deleted
    //hero->detach(this);

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
    FrameBuilderText framebuilder(gfx);

    // TODO: auto frame position
    // - set frame number, size them equally
    frames["Menu"] = framebuilder.createFrame("Menu", "", Point2D(46, 16));
    frames["HeroStat"] = framebuilder.createFrame("Hero Stat", hero->toString(), Point2D(32, 16), Point2D(50, 0));
    frames["EventLog"] = framebuilder.createFrame("Event Log", "", Point2D(32, 16), Point2D(50, 19));

    menu = new Menu();
    menu->addItem(new MenuItem("Attack", new CmdAttack(hero)));
    menu->addItem(new MenuItem("Quit", new CmdQuitApp()));

    menu->draw(frames["Menu"]);
}

int Game::inputHandling()
{
    inputEventNo = 0;

    if (gfx->getKey(VK_RETURN).isPressed) {
        menu->execute();
        ++inputEventNo;
    }
    if (gfx->getKey(VK_UP).isPressed) {
        menu->setSelection(
            menu->getSelection() == 0
                ? 0
                : menu->getSelection() - 1);

        menu->draw(frames["Menu"]);
        ++inputEventNo;
    }
    if (gfx->getKey(VK_DOWN).isPressed) {
        menu->setSelection(
            menu->getSelection() == menu->getItems().size() - 1
                ? menu->getSelection()
                : menu->getSelection() + 1);

        menu->draw(frames["Menu"]);
        ++inputEventNo;
    }

    return inputEventNo;
}

void Game::update(float elapsedTime)
{
    for (const auto& e : frames) {
        e.second->draw();
    }
}

void Game::trigger(Event& evt)
{
    hero->setExperience(hero->getExperience() + std::stoi(evt.msg));
    frames["HeroStat"]->setContent(hero->toString());
    frames["EventLog"]->setContent("Last attack: " + evt.msg);
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
        "Name       : " + _name + '\n' +
        "Level      : " + std::to_string(_heroLevel) + '\n' +
        "Exp        : " + std::to_string(_experience) + '\n' +
        "Min damage : " + std::to_string(_damageMin) + '\n' +
        "Max damage : " + std::to_string(_damageMax)
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

CmdAttack::CmdAttack(Hero* hero)
{
    _obj = hero;
}

void CmdAttack::execute()
{
    _obj->attack();
}

} // End of namespace rpg
} // End of namespace glib