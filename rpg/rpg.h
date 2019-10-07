#pragma once
#include <random>
#include <map>

#include "../gGfx/gGfx.h"
#include "../gGfx/frames.h"

namespace glib {
namespace rpg {

using glib::gGfx::Point2D;


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

    glib::gGfx::Frame* frame = nullptr;
    std::map<std::string, glib::gGfx::Frame*> frames;
    
    Hero* hero = nullptr;
};


class Hero : public Publisher {
public:
    Hero(Game* engine, std::string name);

    void attach(Subscriber* sub) override       { _subs.push_back(sub); }
    void detach(Subscriber* sub) override       { _subs.erase(std::find(_subs.begin(), _subs.end(), sub)); }
    void notify(Event& evt) override            { for (auto& e : _subs) e->trigger(evt); }

    std::string toString() const;

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



} // End of namespace rpg
} // End of namespace glib