#pragma once
#include <map>
#include <optional>

#include "../gGfx/gGfx.h"
#include "../gGfx/frames.h"

extern std::atomic<bool> atomActive;

namespace rts {


using glib::Event;
using glib::Subscriber;
using glib::Publisher;
using glib::Random;
using glib::gGfx::Point2D;
using glib::gGfx::Engine;
using glib::gGfx::EngineGFX;
using glib::UI::Command;
using glib::UI::Frame;
using glib::UI::FrameBuilder;
using glib::UI::FrameBuilderText;
using glib::UI::FrameContent;
using glib::UI::FrameContentText;
using glib::UI::Logger;
using glib::UI::Menu;
using glib::UI::MenuItem;


enum class enu_gender {
    MALE = 0
    , FEMALE = 1
};

class Inhabitant {
    friend class PopulationRM;
public:
    Inhabitant(size_t familyIdParent, std::optional<enu_gender> gender, unsigned short age = 0);

private:
    enu_gender m_gender;
    unsigned short m_age = 0;
    unsigned short m_nChildren = 0;
    size_t m_familyIdParent = 0;
    size_t m_familyId = 0;      // 0 means single
};

struct StatPopulation {
    size_t nFamilySizeMax = 0;
    size_t nFemales = 0;
    size_t nMales = 0;
    size_t nChildren = 0;
    size_t nAdults = 0;
    size_t nElders = 0;
};

class Population {
public:
    virtual void iteratePopulation() = 0;
    virtual size_t getPopulationSize() = 0;
};


class PopulationRM : public Population {
    friend class World;
public:
    PopulationRM(size_t initPopulationSize);
    void iteratePopulation() override;
    size_t getPopulationSize() override           { return m_populationSizeCurrent; }
    size_t getNFamilyFull()     { return std::count_if(m_inhabitants.begin(), m_inhabitants.end(), [&](const auto& x) {return x->m_nChildren == m_familySizeMax; }); }
    size_t getNFemales()        { return std::count_if(m_inhabitants.begin(), m_inhabitants.end(), [&](const auto& x) {return x->m_gender == enu_gender::FEMALE; }); }
    size_t getNMales()          { return std::count_if(m_inhabitants.begin(), m_inhabitants.end(), [&](const auto& x) {return x->m_gender == enu_gender::MALE; }); }
    size_t getNChildren()       { return std::count_if(m_inhabitants.begin(), m_inhabitants.end(), [&](const auto& x) {return x->m_age < m_fertilityAgeMinYear ; }); }
    size_t getNElder()          { return std::count_if(m_inhabitants.begin(), m_inhabitants.end(), [&](const auto& x) {return x->m_age > m_fertilityAgeMinYear; }); }

private:
    void findMate(size_t i);
    void makeChild(size_t i);
    void updateStats(size_t i);

    std::vector<Inhabitant*> m_inhabitants;

    size_t m_populationSizeCurrent = 0;
    size_t m_populationSizeMax = 1'000'000;
    size_t m_familySizeMax = 5;
    size_t m_fertilityAgeMinYear = 20;
    size_t m_fertilityAgeMaxYear = 40;
    size_t m_lifeExpectationAvgYear = 60;
    size_t m_familyIdLast = 0;
    size_t m_iteration = 0;

    StatPopulation m_stats;

};

class World {
    friend class Game;

public:
    World(Population* population);

    void iterate();

    size_t getPopulationSize();

private:
    Population* m_population = nullptr;

    void populate();
};


class Game : public Engine, public Subscriber {
public:
    Game(size_t width, size_t height, size_t fontWidth, size_t fontHeight);
    ~Game();

    void run();
    void initNewWorld()                 { m_world = new World(new PopulationRM(5)); }

protected:
    void init() override;
    int inputHandling() override;
    void update(float elapsedTime) override;
    void trigger(Event& evt) override;

private:
    EngineGFX* m_gfx;
    Random* random = nullptr;
    Logger* logger = nullptr;

    int m_inputEventNo = 0;

    Menu* m_menu = nullptr;
    Frame* m_frame = nullptr;
    World* m_world = nullptr;
    std::map<std::string, Frame*> m_frames;
    float m_gameTimeSec = 0;
    float m_timeSLR = 0;        // Time passed by Since Last Refresh

};


class CmdQuitApp : public Command {
public:
    CmdQuitApp()                        { execute(); }
    void execute() override             { atomActive = false; }
};

class CmdNewWorld : public Command {
public:
    CmdNewWorld(Game* game)
        : m_game(game)                  { execute(); }
    void execute() override             { m_game->initNewWorld(); }

private:
    Game* m_game;
};



} // End of namespace rts