// **********************************************
// ** gkpro @ 2019-10-09                       **
// **                                          **
// **           ---  G-Library  ---            **
// **      User Interface implementation       **
// **                                          **
// **********************************************

#include "pch.h"
#include "frames.h"

extern std::mutex mx;

namespace glib {
namespace UI {


Logger* Logger::m_instance = nullptr;
Frame* Logger::m_output = nullptr;
size_t Logger::m_nLog = 0;
int Frame::_id = 0;

FrameBasic::FrameBasic(EngineGFX* engineGfx
                      ,const Point2D& topleft
                      ,const Point2D& extent
                      ,std::string title
                      ,wchar_t borderType = ' ')
{
    _gfx = engineGfx;
    _topleft = topleft;
    _btmright.x = topleft.x + extent.x;
    _btmright.y = topleft.y + extent.y;
    _extent = extent;
    _title = title;
    _borderType = borderType;
    titleLength = _title.size();
    titlePosX = (size_t)_topleft.x + (size_t)_extent.x / 2 - titleLength / 2;
}

FrameBasic::~FrameBasic()
{
    if (_content) delete _content;
}

void FrameBasic::draw()
{
    // Drawing horizontal lines
    size_t x = (size_t)_extent.x < _gfx->getScreenWidth() ? (size_t)_extent.x : _gfx->getScreenWidth();
    for (size_t i = 0; i <= x; ++i) {
        _gfx->draw((size_t)(_topleft.x + i), (size_t)_topleft.y, COLOUR::FG_GREY, _borderType);
        _gfx->draw((size_t)(_btmright.x - i), (size_t)_btmright.y, COLOUR::FG_GREY, _borderType);
    }
    // Drawing vertical lines
    size_t y = (size_t)_extent.y < _gfx->getScreenHeight() ? (size_t)_extent.y : _gfx->getScreenHeight();
    for (size_t i = 0; i <= y; ++i) {
        _gfx->draw((size_t)_topleft.x, (size_t)(_topleft.y + i), COLOUR::FG_GREY, _borderType);
        _gfx->draw((size_t)_btmright.x, (size_t)(_btmright.y - i), COLOUR::FG_GREY, _borderType);
    }

    // Drawing title
    _gfx->setCurPosX(static_cast<short>(titlePosX));
    _gfx->setCurPosY(static_cast<short>(_topleft.y));
    _gfx->print(" " + _title + " ");

    _content->draw();

    /*
    _gfx->draw((int)_topleft.x,  (int)_topleft.y,  FG_GREY, _borderType);         // Top left corner
    _gfx->draw((int)_btmright.x, (int)_topleft.y,  FG_GREY, _borderType);         // Top right corner
    _gfx->draw((int)_topleft.x,  (int)_btmright.y, FG_GREY, _borderType);         // Bottom left corner
    _gfx->draw((int)_btmright.x, (int)_btmright.y, FG_GREY, _borderType);         // Bottom right corner
    */
}

void FrameBasic::move()
{
    _g_uNIE;
}

void FrameBasic::resize()
{
    _g_uNIE;
}

void FrameBasic::close()
{
    _g_uNIE;
}

void FrameBasic::setTitle()
{
    _g_uNIE;
}

void FrameBasic::setContent(FrameContent* content)
{
    if (_content) delete _content;
    _content = content;
}

void FrameBasic::setContent(std::string content_name)
{
    _content->setContent(content_name);
    _content->markForUpdate();
}

FrameContentText::FrameContentText(Frame* frame, std::string str)
        : _frame(frame), _str(str)
{
    _formatter = new LinebreakSimple(this);
    _width = (size_t)frame->getSize().x - 1 - _padding * 2;
    _height = (size_t)frame->getSize().y - 1;
    frame->setContent(this);
}

FrameContentText::~FrameContentText()
{
    delete _formatter;
}

void FrameContentText::draw()
{
    if (_outdated) {
        refresh();
        _outdated = false;
    }
    print();
}

void FrameContentText::refresh()
{
    format();
    __str_lines = _linebreaks.size();
    __gfx = _frame->getEngine();
}

void FrameContentText::print()
{
    __str_begin = 0;

    for (size_t i = 0; i < __str_lines; ++i) {
        __gfx->setCurPosX(static_cast<short>(
            _frame->getTopLeft().x + _frame->getBorderSize() + _padding));
        __gfx->setCurPosY(static_cast<short>(
            _frame->getTopLeft().y + _frame->getBorderSize() + i));

        __str_length = _linebreaks.at(i) - __str_begin;
        __str_line = _str.substr(__str_begin, __str_length);
        __str_line.erase(std::remove(__str_line.begin(), __str_line.end(), '\n'));
        __gfx->print(__str_line);
        __str_begin = _linebreaks.at(i);
    }
}

void FrameContentText::format()
{
    _formatter->format();
}


void LinebreakSimple::format()
{
    size_t frame_width = _frameContent->getWidth();
    std::string str = _frameContent->getContent();
    size_t strwidth = str.size();
    std::vector<size_t> linebreaks;

    // Interpret \n as linebreak
    for (size_t i = 0; i < strwidth; ++i) {
        if (str[i] == '\n' && i != strwidth) {
            linebreaks.push_back(i);
        }
    }

    // Get the length between linebreaks and continue breaking lines if needed
    size_t lines = linebreaks.size();
    size_t prev_break = 0;
    size_t distance = 0;
    for (size_t i = 0; i < lines; ++i) {
        distance = linebreaks.at(i) - prev_break;
        while (distance > frame_width) {
            linebreaks.push_back(prev_break + frame_width);
            distance -= frame_width;
        }
        prev_break = linebreaks.at(i);
    }

    // End string with linebreak
    linebreaks.push_back(strwidth);

    _frameContent->setLinebreaks(linebreaks);
}


FrameBuilder::FrameBuilder(EngineGFX* engine) : _engine(engine)
{
    _lastFramePos = Point2D(0, 0);
    _frameCount = 0;
}

Frame* FrameBuilder::createFrame(std::string title, Point2D extent, Point2D topleft)
{
    _lastFramePos = Point2D(topleft.x + extent.x + _padding, topleft.y + extent.y + _padding);
    Frame* frame = new FrameBasic(_engine, topleft, extent, title, '#');
    new FrameContentText(frame, std::string("..."));

    ++_frameCount;
    return frame;

}

Frame* FrameBuilder::createFrame(std::string title, Point2D extent)
{
    Point2D p(_lastFramePos.x, 0);
    return createFrame(title, extent, p);
}

Frame* FrameBuilder::createFrame(std::string title)
{
    Point2D p(_lastFramePos.x, 0);
    Point2D e(
         static_cast<float>(frameDimensionLimits.minWidth)
        ,static_cast<float>(frameDimensionLimits.minHeight));
    return createFrame(title, e, p);
}

Frame* FrameBuilderText::createFrame(std::string title, std::string text, Point2D extent, Point2D topleft)
{
    Frame* frame = FrameBuilder::createFrame(title, extent, topleft);
    new FrameContentText(frame, text);
    return frame;
}

Frame* FrameBuilderText::createFrame(std::string title, std::string text, Point2D extent)
{
    Frame* frame = FrameBuilder::createFrame(title, extent);
    new FrameContentText(frame, text);
    return frame;
}

Frame* FrameBuilderText::createFrame(std::string title, std::string text)
{
    Frame* frame = FrameBuilder::createFrame(title);
    new FrameContentText(frame, text);
    return frame;
}

Menu::Menu()
{

}

Menu::~Menu()
{
    for (auto& e : _items) delete e;
}

void Menu::addItem(MenuItem* item)
{
    _items.push_back(item);
}


void Menu::draw(Frame* frame)
{
    std::string str;
    size_t length = _items.size();
    for (size_t i = 0; i < length; ++i) {
        str += _items[i]->toString();
        if (_selection == i) {
            str += " #\n";
        }
        else {
            str += "\n";
        }
    }
    frame->setContent(str);
}

void Menu::execute()
{
    _items.at(_selection)->execute();
}

void Menu::setSelection(size_t s)
{
    _selection = s;
}

size_t Menu::getSelection()
{
    return _selection;
}

std::vector<MenuItem*> Menu::getItems()
{
    return _items;
}



Logger::Logger()
{

}

Logger* Logger::getInstance()
{
    if (m_instance == nullptr) {
        m_instance = new Logger();
    }
    return m_instance;
}

void Logger::attachOutput(Frame* output)
{
    m_output = output;
}

void Logger::log(const std::string& msg)
{
    m_output->setContent(
        std::to_string(++m_nLog)
        + ": " + msg
    );
}



} // End of namespace gGfx
} // End of namespace glib