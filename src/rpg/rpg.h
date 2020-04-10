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


enum class ItemType {
    ONE_HANDED_SWORD,
    BOW
};

enum class ItemSlotType {
    MAIN_HAND,
    OFF_HAND,
    HEAD,
    TORSO,
    HANDS,
    WAIST,
    LEGS,
    FEET,
    NECK,
    FINGER
};

enum class ElementalType {
    PHYSICAL,
    FIRE,
    ICE
};

enum class DamageType {
    MELEE,
    PROJECTILE,
    SPELL
};

struct damage {
    DamageType type;
    ElementalType elemental;
    int min;
    int max;
    float duration;
};

struct stat {
    int life;
    int mana;
    damage dmg;

};



class Hero;

class Game : public glib::gGfx::Engine, public Subscriber {
public:
    Game(size_t width, size_t height, size_t fontWidth, size_t fontHeight);
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
    int _hp;
    int _mana;

    stat stats;
    int _damageMin;
    int _damageMax;
};


class Item {
public:
    Item(std::string name) : _name(name) { generateID(); }

protected:
    std::string _name;

private:
    void generateID()           { ++_uid; }
    static int _uid;

};

class Equipment : public Item {
public:
    Equipment(std::string name, stat stats) : Item(name), _stats(stats) {}
    void equip();

private:
    stat _stats;
    //ItemSlotType slotType;
    //ItemType type;
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