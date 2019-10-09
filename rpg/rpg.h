#pragma once
#include <random>
#include <map>

#include "../gGfx/gGfx.h"
#include "../gGfx/frames.h"

extern std::atomic<bool> atomActive;

namespace glib {
namespace rpg {

using glib::gGfx::Point2D;
using glib::UI::Command;
using glib::UI::Frame;
using glib::UI::FrameBuilder;
using glib::UI::FrameBuilderText;
using glib::UI::FrameContent;
using glib::UI::FrameContentText;
using glib::UI::Menu;
using glib::UI::MenuItem;


class Hero;

class Game : public glib::gGfx::Engine, public Subscriber {
public:
    Game(gint width, gint height, gint fontWidth, gint fontHeight);
    ~Game();
    void run();

    std::mt19937& rng() { return mt; }

protected:
    void init() override;
    int inputHandling() override;
    void update(float elapsedTime) override;
    void trigger(Event& evt) override;

private:
    glib::gGfx::EngineGFX* gfx;
    std::random_device rd;
    std::mt19937 mt;

    int inputEventNo = 0;

    Menu* menu = nullptr;
    Frame* frame = nullptr;
    std::map<std::string, Frame*> frames;
    
    Hero* hero = nullptr;
};


class Hero : public Publisher {
public:
    Hero(Game* engine, std::string name);

    std::string toString() const;
    int getExperience()                         { return _experience; }
    void setExperience(int exp)                 { _experience = exp; }

    void move(glib::gGfx::DIRECTION_2D direction);
    int attack();

private:
    Game* _engine;
    std::string _name;
    int _experience;
    int _heroLevel;
    int _maxHp;
    int _hp;
    int _maxMana;
    int _mana;

    int _damageMin;
    int _damageMax;
};

class CmdQuitApp : public Command {
public:
    CmdQuitApp()                { execute(); }
    void execute() override     { atomActive = false; }
};

class CmdAttack : public Command {
public:
    CmdAttack(Hero* hero);
    void execute() override;

private:
    Hero* _obj;
};


} // End of namespace rpg
} // End of namespace glib