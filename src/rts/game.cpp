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


Game::Game(size_t width, size_t height, size_t fontWidth, size_t fontHeight)
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
        m_gameTimeSec += m_timeSLR;
        --m_timeSLR;
        m_world->iterate();
    }

    // Refresh UI
    m_frames["Stat"]->setContent(
          "Time      : " + std::to_string(m_gameTimeSec) + '\n'
        + "Population: " + std::to_string(m_world->getPopulationSize()) + '\n'
        + "FullFamily: " + std::to_string(dynamic_cast<PopulationRM*>(m_world->m_population)->getNFamilyFull()) + '\n'
        + "Females   : " + std::to_string(dynamic_cast<PopulationRM*>(m_world->m_population)->getNFemales()) + '\n'
        + "Males     : " + std::to_string(dynamic_cast<PopulationRM*>(m_world->m_population)->getNMales()) + '\n'
        + "Children  : " + std::to_string(dynamic_cast<PopulationRM*>(m_world->m_population)->getNChildren()) + '\n'
        + "Elders    : " + std::to_string(dynamic_cast<PopulationRM*>(m_world->m_population)->getNElder())
    );

    for (const auto& e : m_frames) {
        e.second->draw();
    }
}

void Game::trigger(Event& evt)
{

}

Inhabitant::Inhabitant(size_t familyIdParent, std::optional<enu_gender> gender, unsigned short age)
{
    if (!gender.has_value()) {
        m_gender = static_cast<enu_gender>(Random::randomInt(0, 1));
    }
    else {
        m_gender = gender.value();
    }
    m_familyIdParent = familyIdParent;
    m_familyId = familyIdParent;
    m_age = age;
}

PopulationRM::PopulationRM(size_t initPopulationSize)
{
    m_populationSizeCurrent = initPopulationSize * 2;

    for (size_t i = 0; i < initPopulationSize; ++i) {
        ++m_familyIdLast;
        m_inhabitants.emplace_back(
            new Inhabitant(m_familyIdLast
                , std::optional<enu_gender>() = enu_gender::FEMALE, Random::randomInt(20, 30))
        );
        ++m_stats.nFemales;
        m_inhabitants.emplace_back(
            new Inhabitant(m_familyIdLast
                , std::optional<enu_gender>() = enu_gender::FEMALE, Random::randomInt(20, 30))
        );
        ++m_stats.nMales;
    }
}

void PopulationRM::iteratePopulation()
{
    ++m_iteration;
    size_t length = m_inhabitants.size();
    for (size_t i = 0; i < length; ++i) {
        ++m_inhabitants[i]->m_age;

        //updateStats();

        if (m_inhabitants[i]->m_age >= m_fertilityAgeMinYear
            && m_inhabitants[i]->m_age <= m_fertilityAgeMaxYear)
        {
            if (m_inhabitants[i]->m_familyId == m_inhabitants[i]->m_familyIdParent) {
                // Detach from parent family
                m_inhabitants[i]->m_familyId = 0;
            }
            if (m_inhabitants[i]->m_familyId == 0) {
                findMate(i);
            }
            else {
                makeChild(i);
            }
        }


    }
    m_inhabitants.erase(
        std::remove_if(m_inhabitants.begin(), m_inhabitants.end()
            , [&](const auto& x) {
                if (x->m_age > m_lifeExpectationAvgYear) {
                    --m_populationSizeCurrent;
                    return true;
                }
                return false;
            })
        ,m_inhabitants.end()
    );
        //m_inhabitants[i]->m_age > m_lifeExpectationAvgYear
}

void PopulationRM::findMate(size_t i)
{
    Logger::log("Finding mate... -Not Implemented-"
        + std::to_string(m_iteration)
    );
    auto mate = std::find_if(m_inhabitants.begin(), m_inhabitants.end()
        , [&](const auto& x) {
            return x->m_familyId == 0
                && x->m_gender != m_inhabitants[i]->m_gender;
        }
    );

    ++m_familyIdLast;
    m_inhabitants[i]->m_familyId = m_familyIdLast;
    if (mate != std::end(m_inhabitants)) {
        (*mate)->m_familyId = m_familyIdLast;
    }
}

void PopulationRM::makeChild(size_t i)
{
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
        ++m_stats.nChildren;
    }
}

void PopulationRM::updateStats(size_t i)
{
    if (m_inhabitants[i]->m_age == m_fertilityAgeMinYear) {
        --m_stats.nChildren;
        ++m_stats.nAdults;
    }
    else if (m_inhabitants[i]->m_age == m_fertilityAgeMaxYear + 1) {
        --m_stats.nAdults;
        ++m_stats.nElders;
    }
}

World::World(Population* population)
{
    m_population = population;
}

void World::populate()
{
    m_population->iteratePopulation();
}

void World::iterate()
{
    populate();
}

size_t World::getPopulationSize()
{
    return m_population->getPopulationSize();
}


} // End of namespace rts