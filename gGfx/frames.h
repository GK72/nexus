#pragma once

#include "gGfx.h"

namespace glib {
namespace gGfx {

class FrameContent;

class Formatter {
public:
    virtual void format() = 0;

};


class Frame {
public:
    virtual ~Frame() {}

    virtual void close() = 0;
    virtual void draw() = 0;
    virtual void resize() = 0;
    virtual void move() = 0;

    virtual void setTitle() = 0;
    virtual void setContent(FrameContent* content) = 0;

    virtual int getID() { return _id; }
    virtual std::string getTitle() = 0;
    virtual Point2D getSize() = 0;
    virtual Point2D getTopLeft() = 0;
    virtual EngineGFX* getEngine() = 0;

protected:
    Frame() { generateID(); }

    static int _id;
    int generateID() { return ++_id; }

};


class FrameContent {
public:
    virtual void draw() = 0;
    virtual void format() = 0;
    virtual std::string getContent() = 0;
    virtual size_t getWidth() = 0;
    virtual void setLinebreaks(std::vector<int> linebreaks) = 0;

};


class FrameBasic : public Frame {
public:
    FrameBasic(EngineGFX* gfx, const Point2D& topleft, const Point2D& extent);
    ~FrameBasic();

    void draw() override;
    void move() override;
    void resize() override;
    void close() override;

    void setTitle() override;
    void setContent(FrameContent* content);

    std::string getTitle() override     { return _title; }
    EngineGFX* getEngine() override     { return _gfx; }
    Point2D getSize() override          { return _extent; }
    Point2D getTopLeft() override       { return _topleft; }

private:
    std::string _title;
    Point2D _topleft;
    Point2D _btmright;
    Point2D _extent;
    wchar_t _borderType = '#';

    FrameContent* _content = nullptr;
    EngineGFX* _gfx = nullptr;

};


class FrameContentText : public FrameContent {
public:
    FrameContentText(Frame* frame, std::string str);
    ~FrameContentText();
    void draw() override;
    void format() override;
    
    std::string getContent() override   { return _str; }
    size_t getWidth() override          { return _width; }

    void setLinebreaks(std::vector<int> linebreaks) { _linebreaks = linebreaks; }

private:
    Frame* _frame = nullptr;
    Formatter* _formatter;
    std::string _str;
    size_t _width;
    size_t _height;
    size_t _padding = 1;
    std::vector<int> _linebreaks;
};


class LinebreakSimple : public Formatter {
public:
    LinebreakSimple(FrameContent* content) : _frameContent(content) {}
    void format() override;

private:
    FrameContent* _frameContent;
};


class FrameBuilder {
public:
    FrameBuilder(EngineGFX* engine) : _engine(engine) {}
    Frame* createFrame();

private:
    EngineGFX* _engine;

};



} // End of namespace gGfx
} // End of namespace glib