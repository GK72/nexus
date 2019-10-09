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
    titlePosX = (gint)_topleft.x + (gint)_extent.x / 2 - titleLength / 2;
}

FrameBasic::~FrameBasic()
{
    if (_content) delete _content;
}

void FrameBasic::draw()
{
    // Drawing horizontal lines
    gint x = (gint)_extent.x < _gfx->getScreenWidth() ? (gint)_extent.x : _gfx->getScreenWidth();
    for (gint i = 0; i <= x; ++i) {
        _gfx->draw((gint)(_topleft.x + i), (gint)_topleft.y, COLOUR::FG_GREY, _borderType);
        _gfx->draw((gint)(_btmright.x - i), (gint)_btmright.y, COLOUR::FG_GREY, _borderType);
    }
    // Drawing vertical lines
    gint y = (gint)_extent.y < _gfx->getScreenHeight() ? (gint)_extent.y : _gfx->getScreenHeight();
    for (gint i = 0; i <= y; ++i) {
        _gfx->draw((gint)_topleft.x, (gint)(_topleft.y + i), COLOUR::FG_GREY, _borderType);
        _gfx->draw((gint)_btmright.x, (gint)(_btmright.y - i), COLOUR::FG_GREY, _borderType);
    }

    // Drawing title
    _gfx->setCurPosX(titlePosX);
    _gfx->setCurPosY(_topleft.y);
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
    _width = (gint)frame->getSize().x - 1 - _padding * 2;
    _height = (gint)frame->getSize().y - 1;
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

    for (gint i = 0; i < __str_lines; ++i) {
        __gfx->setCurPosX(_frame->getTopLeft().x + _frame->getBorderSize() + _padding);
        __gfx->setCurPosY(_frame->getTopLeft().y + _frame->getBorderSize() + i);

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
    gint frame_width = _frameContent->getWidth();
    std::string str = _frameContent->getContent();
    gint strwidth = str.size();
    std::vector<gint> linebreaks;

    // Interpret \n as linebreak
    for (gint i = 0; i < strwidth; ++i) {
        if (str[i] == '\n' && i != strwidth) {
            linebreaks.push_back(i);
        }
    }

    // Get the length between linebreaks and continue breaking lines if needed
    gint lines = linebreaks.size();
    gint prev_break = 0;
    gint distance = 0;
    for (gint i = 0; i < lines; ++i) {
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
    Point2D e(frameDimensionLimits.minWidth, frameDimensionLimits.minHeight);
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
    gint length = _items.size();
    for (gint i = 0; i < length; ++i) {
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

void Menu::setSelection(gint s)
{
    _selection = s;
}

gint Menu::getSelection()
{
    return _selection;
}

std::vector<MenuItem*> Menu::getItems()
{
    return _items;
}



} // End of namespace gGfx
} // End of namespace glib