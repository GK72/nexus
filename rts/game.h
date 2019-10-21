#pragma once
#include <map>
#include <optional>

#include "../gGfx/gGfx.h"
#include "../gGfx/frames.h"

extern std::atomic<bool> atomActive;

namespace rts {


using glib::gint;
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
    friend class Population;
public:
    Inhabitant(gint familyid, std::optional<enu_gender> gender, unsigned short age = 0);

private:
    enu_gender m_gender;
    unsigned short m_age = 0;
    unsigned short m_nChildren = 0;
    gint m_familyId = 0;      // Single
};

class Population {
    friend class World;
public:
    Population(gint initPopulationSize);
    void iteratePopulation();

private:
    void findMate(Inhabitant* inhabitant);

    std::vector<Inhabitant*> m_inhabitants;

    gint m_populationSizeCurrent = 0;
    gint m_populationSizeMax = 1'000'000;
    gint m_familySizeMax = 5;
    gint m_fertilityAgeMinYear = 20;
    gint m_fertilityAgeMaxYear = 40;
    gint m_lifeExpectationAvgYear = 60;
    gint m_familyIdLast = 0;

};

class World {
    friend class Game;

public:
    World();

    void iterate();

    gint getPopulationSize();

private:
    Population* m_population = nullptr;
    
    void populate();
};


class Game : public Engine, public Subscriber {
public:
    Game(gint width, gint height, gint fontWidth, gint fontHeight);
    ~Game();

    void run();
    void initNewWorld()                 { m_world = new World(); }

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