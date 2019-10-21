// **********************************************
// ** gkpro @ 2019-10-20                       **
// **                                          **
// **                  Game                    **
// **              ---  RTS  ---               **
// **                                          **
// **                                          **
// **********************************************

#include "game.h"

std::mutex mx;

namespace rts {


Game::Game(gint width, gint height, gint fontWidth, gint fontHeight)
{
#ifdef _MSC_BUILD
    m_gfx = new glib::gGfx::EngineConW(this, (int)width, (int)height, (int)fontWidth, (int)fontHeight);
#else
    gfx = new glib::gGfx::EngineCurses((int)width, (int)height, (int)fontWidth, (int)fontHeight);
#endif
}

Game::~Game()
{
    delete m_gfx;
    if (m_frame) delete m_frame;
}

void Game::run()
{
    m_gfx->run();
}

void Game::init()
{
    random = Random::getInstance();
    logger = Logger::getInstance();

    FrameBuilderText framebuilder(m_gfx);

    // TODO: auto frame position
    // - set frame number, size them equally
    m_frames["Menu"] = framebuilder.createFrame("Menu", "", Point2D(46, 16));
    m_frames["Stat"] = framebuilder.createFrame("World Statistics", "", Point2D(32, 16), Point2D(50, 0));
    m_frames["EventLog"] = framebuilder.createFrame("Event Log", "<empty log>", Point2D(32, 16), Point2D(50, 20));
    logger->attachOutput(m_frames["EventLog"]);

    m_menu = new Menu();
    m_menu->addItem(new MenuItem("Start world", new CmdNewWorld(this)));
    m_menu->addItem(new MenuItem("Quit", new CmdQuitApp()));

    m_menu->draw(m_frames["Menu"]);
}

int Game::inputHandling()
{
    m_inputEventNo = 0;

    if (m_gfx->getKey(VK_RETURN).isPressed) {
        m_menu->execute();
        ++m_inputEventNo;
    }
    if (m_gfx->getKey(VK_UP).isPressed) {
        m_menu->setSelection(
            m_menu->getSelection() == 0
            ? 0
            : m_menu->getSelection() - 1);

        m_menu->draw(m_frames["Menu"]);
        ++m_inputEventNo;
    }
    if (m_gfx->getKey(VK_DOWN).isPressed) {
        m_menu->setSelection(
            m_menu->getSelection() == m_menu->getItems().size() - 1
            ? m_menu->getSelection()
            : m_menu->getSelection() + 1);

        m_menu->draw(m_frames["Menu"]);
        ++m_inputEventNo;
    }

    return m_inputEventNo;
}

void Game::update(float elapsedTime)
{
    m_timeSLR += elapsedTime;
    if (m_timeSLR > 1) {
        --m_timeSLR;
        m_gameTimeSec += elapsedTime;
        m_world->iterate();
    }

    // Refresh UI
    m_frames["Stat"]->setContent("Population: "
        + std::to_string(m_world->getPopulationSize()));

    for (const auto& e : m_frames) {
        e.second->draw();
    }
}

void Game::trigger(Event& evt)
{

}

Inhabitant::Inhabitant(gint familyid, std::optional<enu_gender> gender, unsigned short age)
{
    if (!gender.has_value()) {
        m_gender = static_cast<enu_gender>(Random::randomInt(0, 1));
    }
    else {
        m_gender = gender.value();
    }
    m_familyId = familyid;
    m_age = age;
}

Population::Population(gint initPopulationSize)
{
    m_populationSizeCurrent = initPopulationSize * 2;

    Inhabitant* inhabitant;
    for (gint i = 0; i < initPopulationSize; ++i) {
        ++m_familyIdLast;
        inhabitant = new Inhabitant(m_familyIdLast, std::optional<enu_gender>() = enu_gender::FEMALE, Random::randomInt(20, 30));
        m_inhabitants.push_back(inhabitant);
        inhabitant = new Inhabitant(m_familyIdLast, std::optional<enu_gender>() = enu_gender::MALE, Random::randomInt(20, 30));
        m_inhabitants.push_back(inhabitant);
    }
}



void Population::iteratePopulation()
{
    gint length = m_inhabitants.size();
    for (gint i = 0; i < length; ++i) {
        ++m_inhabitants[i]->m_age;
        if (m_inhabitants[i]->m_familyId == 0) {
            //findMate(e);
            Logger::log("Finding mate... -Not Implemented-");
        }
        else {
            // Make child
            if (m_inhabitants[i]->m_age >= m_fertilityAgeMinYear
                    && m_inhabitants[i]->m_age <= m_fertilityAgeMaxYear
                    && m_inhabitants[i]->m_nChildren < m_familySizeMax)
            {
                if (m_inhabitants[i]->m_gender == enu_gender::FEMALE) {
                    m_inhabitants.push_back(new Inhabitant(
                        m_inhabitants[i]->m_familyId, std::optional<enu_gender>()));
                        ++m_populationSizeCurrent;
                }
                ++m_inhabitants[i]->m_nChildren;
            }
        }
        if (m_inhabitants[i]->m_age >= m_fertilityAgeMinYear
                && m_inhabitants[i]->m_nChildren == 0) {
            m_inhabitants[i]->m_familyId = 0;
        }
    }
    std::remove_if(m_inhabitants.begin(), m_inhabitants.end()
        , [&](const auto& x) {
            if (x->m_age > m_lifeExpectationAvgYear) {
                --m_populationSizeCurrent;
                return true;
            }
            return false;
        });
        //m_inhabitants[i]->m_age > m_lifeExpectationAvgYear
}

void Population::findMate(Inhabitant* inhabitant)
{
    auto mate = std::find_if(m_inhabitants.begin(), m_inhabitants.end()
        , [&](const auto& x) {
            return x->m_gender == inhabitant->m_gender
                && x->m_familyId == 0; });

    ++m_familyIdLast;
    inhabitant->m_familyId = m_familyIdLast;
    //mate->m_familyId = m_familyIdLast;
}

World::World()
{
    m_population = new Population(5);
}

void World::populate()
{
    m_population->iteratePopulation();
}

void World::iterate()
{
    populate();
}

gint World::getPopulationSize()
{
    return m_population->m_populationSizeCurrent;
}


} // End of namespace rts