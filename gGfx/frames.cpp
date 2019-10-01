#include "pch.h"
#include "frames.h"

//void glib::gGfx::FrameContentText::set(Frame* frame, std::string str)
//{
//    _frame = frame;
//    _str = str;
//}

glib::gGfx::FrameContentText::FrameContentText(Frame* frame, std::string str)
{
    _frame = frame;
    _str = str;
}

void glib::gGfx::FrameContentText::draw()
{
    size_t length_x = _frame->getSize().x;
    size_t length_y = _frame->getSize().y;
    std::string tmp;
    size_t begin = 0;
    size_t str_length = length_x;
    size_t lines = _str.length() / length_x;

    _frame->getEngine()->setCurPosX(_frame->getTopLeft().x + 1);
    _frame->getEngine()->setCurPosY(_frame->getTopLeft().y + 1);

    for (size_t i = 0; i < lines; ++i) {
        begin = i * length_x;
        tmp = _str.substr(begin, str_length);
        _frame->getEngine()->print(tmp);
        _frame->getEngine()->printn();
        // TODO: Only the first line showing (and it needs to be shifted one char left)
    }
}

glib::gGfx::FrameBasic::FrameBasic(EngineGFX* engineGfx, const Point2D& topleft, const Point2D& extent)
{
    _gfx = engineGfx;
    _topleft = topleft;
    _btmright.x = topleft.x + extent.x;
    _btmright.y = topleft.y + extent.y;
    _extent = extent;
}

void glib::gGfx::FrameBasic::draw()
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

void glib::gGfx::FrameBasic::move()
{
    _g_uNIE;
}

void glib::gGfx::FrameBasic::resize()
{
    _g_uNIE;
}

void glib::gGfx::FrameBasic::setTitle()
{
    _g_uNIE;
}

void glib::gGfx::FrameBasic::setContent(FrameContent* content)
{
    _content = content;
}
