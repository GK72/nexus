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

    // TODO: auto frame position
    // - set frame number, size them equally
    frames["Menu"] = framebuilder.createFrame("Menu", Point2D(46, 16));
    frames["HeroStat"] = framebuilder.createFrame("Hero Stat", Point2D(32, 16), Point2D(50, 0));
    frames["EventLog"] = framebuilder.createFrame("Event Log", Point2D(32, 16), Point2D(50, 19));

    // TODO: wrap these in builder
    new glib::gGfx::FrameContentText(frames["Menu"], "Menu ...");
    menu = new glib::gGfx::Menu();
    menu->addItem(new glib::gGfx::MenuItem("Attack", new CmdAttack(hero)));
    menu->addItem(new glib::gGfx::MenuItem("Quit", new CmdQuitApp()));

    // Menu toString
    std::string menuStr;
    for (auto& e : menu->getItems()) {
        menuStr += e->toString() + '\n';
    }
    frames["Menu"]->setContent(menuStr);


    new glib::gGfx::FrameContentText(frames["HeroStat"], hero->toString());
    new glib::gGfx::FrameContentText(frames["EventLog"], "");
}

int Game::inputHandling()
{
    inputEventNo = 0;

    // TODO: send event to display the new selection
    //       and handle elsewhere the printing
    if (gfx->getKey(VK_RETURN).isPressed) {
        menu->execute();
        ++inputEventNo;
    }
    if (gfx->getKey(VK_UP).isPressed) {
        menu->setSelection(
            menu->getSelection() == 0
                ? 0
                : menu->getSelection() - 1);

        std::string menuStr;
        gint i = 0;
        for (auto& e : menu->getItems()) {
            menuStr += e->toString();
            if (menu->getSelection() == i) {
                menuStr += " #\n";
            }
            else {
                menuStr += "\n";
            }
            ++i;
        }
        frames["Menu"]->setContent(menuStr);

        ++inputEventNo;
    }
    if (gfx->getKey(VK_DOWN).isPressed) {
        menu->setSelection(
            menu->getSelection() == menu->getItems().size() - 1
                ? menu->getSelection()
                : menu->getSelection() + 1);

        std::string menuStr;
        gint i = 0;
        for (auto& e : menu->getItems()) {
            menuStr += e->toString();
            if (menu->getSelection() == i) {
                menuStr += " #\n";
            }
            else {
                menuStr += "\n";
            }
            ++i;
        }
        frames["Menu"]->setContent(menuStr);

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