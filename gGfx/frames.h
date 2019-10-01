#pragma once

#include "gGfx.h"

namespace glib {
namespace gGfx {

class FrameContent;

class Frame
{
public:
    virtual void draw() {}
    virtual void move() {}
    virtual void resize() {}

    virtual void setTitle() {}
    virtual void setContent(FrameContent* content) {}
    virtual Point2D getSize() { return Point2D(0,0); }
    virtual Point2D getTopLeft() { return Point2D(0,0); }
    virtual EngineGFX* getEngine() { return nullptr; }


};

class FrameContent
{
public:
    //virtual void set(Frame* frame, std::string str) = 0;
    virtual void draw() = 0;

};

class FrameContentText : public FrameContent
{
public:
    FrameContentText(Frame* frame, std::string str);
    //void set(Frame* frame, std::string str) override;
    void draw() override;

private:
    Frame* _frame = nullptr;
    std::string _str;
};

class FrameBasic : public Frame
{
public:
    FrameBasic(EngineGFX* gfx, const Point2D& topleft, const Point2D& extent);

    void draw() override;
    void move() override;
    void resize() override;

    void setTitle() override;
    void setContent(FrameContent* content);

    EngineGFX* getEngine() override { return _gfx; }
    Point2D getSize() override { return _extent; }
    Point2D getTopLeft() override { return _topleft; }


private:
    std::string _title;
    Point2D _topleft;
    Point2D _btmright;
    Point2D _extent;
    wchar_t _borderType = '#';

    FrameContent* _content;
    EngineGFX* _gfx;

};



} // End of namespace gGfx
} // End of namespace glib