#include "pch.h"
#include "frames.h"

namespace glib {
namespace gGfx {


int Frame::_id = 0;

FrameBasic::FrameBasic(EngineGFX* engineGfx, const Point2D& topleft, const Point2D& extent)
{
    _gfx = engineGfx;
    _topleft = topleft;
    _btmright.x = topleft.x + extent.x;
    _btmright.y = topleft.y + extent.y;
    _extent = extent;
}

FrameBasic::~FrameBasic()
{
    if (_content) delete _content;
}

void FrameBasic::draw()
{
    // Drawing horizontal lines
    size_t x = (size_t)_extent.x;
    for (size_t i = 0; i <= x; ++i) {
        _gfx->draw((int)(_topleft.x + i), (int)_topleft.y, FG_GREY, _borderType);
        _gfx->draw((int)(_btmright.x - i), (int)_btmright.y, FG_GREY, _borderType);
    }
    // Drawing vertical lines
    size_t y = (size_t)_extent.y;
    for (size_t i = 0; i <= y; ++i) {
        _gfx->draw((int)_topleft.x, (int)(_topleft.y + i), FG_GREY, _borderType);
        _gfx->draw((int)_btmright.x, (int)(_btmright.y - i), FG_GREY, _borderType);
    }

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
    _content = content;
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
    format();
    EngineGFX* g = _frame->getEngine();
    size_t lines = _linebreaks.size();
    std::string line;
    size_t begin = 0;
    size_t str_length = 0;

    for (size_t i = 0; i < lines; ++i) {
        g->setCurPosX(_frame->getTopLeft().x + _padding);
        g->setCurPosY(_frame->getTopLeft().y + i + 1);

        str_length = _linebreaks.at(i) - begin;
        line = _str.substr(begin, str_length);
        g->print(line);
        begin = _linebreaks.at(i);
    }
}

void FrameContentText::format()
{
    _formatter->format();
}


void LinebreakSimple::format()
{
    size_t width = _frameContent->getWidth();
    size_t strwidth = _frameContent->getContent().size();
    size_t count = strwidth / width + 1;
    std::vector<int> linebreaks;

    for (size_t i = 0; i < count; ++i) {
        linebreaks.push_back(width * (i + 1));
    }

    _frameContent->setLinebreaks(linebreaks);
}


Frame* FrameBuilder::createFrame()
{
    Point2D p(0, 5);
    Point2D q(50, 15);
    Frame* frame = new FrameBasic(_engine, p, q);
    new FrameContentText(frame, std::string("Frame Builder test"));

    return frame;
}

} // End of namespace gGfx
} // End of namespace glib