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

    virtual int getID() const               { return _id; }
    virtual std::string getTitle() const    = 0;
    virtual gint getBorderSize() const      = 0;
    virtual Point2D getSize() const         = 0; 
    virtual Point2D getTopLeft() const      = 0;
    virtual EngineGFX* getEngine() const    = 0;

protected:
    Frame() { generateID(); }

    static int _id;
    int generateID() { return ++_id; }

};


class FrameContent {
public:
    virtual void draw() = 0;
    virtual void format() = 0;
    virtual std::string getContent() const = 0;
    virtual gint getWidth() const = 0;
    virtual void setLinebreaks(std::vector<int> linebreaks) = 0;

};


class FrameBasic : public Frame {
public:
    FrameBasic(EngineGFX* gfx, const Point2D& topleft, const Point2D& extent, std::string title, wchar_t borderType);
    ~FrameBasic();

    void draw() override;
    void move() override;
    void resize() override;
    void close() override;

    void setTitle() override;
    void setContent(FrameContent* content);

    std::string getTitle() const override   { return _title; }
    EngineGFX* getEngine() const override   { return _gfx; }
    gint getBorderSize() const override     { return _borderSize; }
    Point2D getSize() const override        { return _extent; }
    Point2D getTopLeft() const override     { return _topleft; }

private:
    std::string _title;
    gint titleLength;
    gint titlePosX;

    Point2D _topleft;
    Point2D _btmright;
    Point2D _extent;
    wchar_t _borderType;
    gint _borderSize = 1;

    FrameContent* _content = nullptr;
    EngineGFX* _gfx = nullptr;

};


class FrameContentText : public FrameContent {
public:
    FrameContentText(Frame* frame, std::string str);
    ~FrameContentText();
    void draw() override;
    void format() override;
    
    std::string getContent() const override     { return _str; }
    gint getWidth() const override              { return _width; }

    void setLinebreaks(std::vector<int> linebreaks) { _linebreaks = linebreaks; }

private:
    Frame* _frame = nullptr;
    Formatter* _formatter;
    std::string _str;
    gint _width;
    gint _height;
    gint _padding = 1;
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
    FrameBuilder(EngineGFX* engine);
    Frame* createFrame(std::string title, Point2D extent, Point2D topleft);
    Frame* createFrame(std::string title, Point2D extent);
    Frame* createFrame(std::string title);

private:
    EngineGFX* _engine;
    gint _frameCount;

    Point2D _lastFramePos;
    gint _padding = 2;

    struct dimensionLimits {
        gint minWidth  = 16;
        gint minHeight = 8;
        gint maxWidth  = 64;
        gint maxHeight = 32;
    } frameDimensionLimits;

};



} // End of namespace gGfx
} // End of namespace glib