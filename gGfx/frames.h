// **********************************************
// ** gkpro @ 2019-10-09                       **
// **                                          **
// **           ---  G-Library  ---            **
// **          User Interface header           **
// **                                          **
// ** Commands                                 **
// ** Frames                                   **
// ** Menu                                     **
// **                                          **
// **********************************************

#pragma once

#include "gGfx.h"

namespace glib {
namespace UI {

using glib::NotImplementedException;
using glib::gGfx::COLOUR;
using glib::gGfx::EngineGFX;
using glib::gGfx::Point2D;


// Forward declarations

class FrameContent;
class MenuItem;

// Interfaces

class Command {
public:
    Command() {}
    virtual void execute() = 0;
};

class Formatter {
public:
    virtual void format() = 0;

protected:
    Formatter() {}
};

class Frame {
public:
    virtual ~Frame() {}

    virtual void close() = 0;
    virtual void draw() = 0;
    virtual void resize() = 0;
    virtual void move() = 0;

    virtual void markForUpdate() = 0;
    virtual void setTitle() = 0;
    virtual void setContent(FrameContent* content) = 0;
    virtual void setContent(std::string content_name) = 0;

    virtual int getID() const               { return _id; }
    virtual FrameContent* getContent()      = 0;
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

    virtual void markForUpdate() = 0;
    virtual void setContent(std::string content) = 0;
    virtual void setLinebreaks(std::vector<gint> linebreaks) = 0;

    virtual std::string getContent() const = 0;
    virtual gint getWidth() const = 0;

protected:
    FrameContent() {}
};


// Implementations

class FrameBasic : public Frame {
public:
    FrameBasic(EngineGFX* gfx, const Point2D& topleft, const Point2D& extent
              ,std::string title, wchar_t borderType);
    ~FrameBasic();

    void draw() override;
    void move() override;
    void resize() override;
    void close() override;

    void markForUpdate() override           { _content->markForUpdate(); }
    void setTitle() override;
    void setContent(FrameContent* content) override;
    void setContent(std::string content_name) override;

    std::string getTitle() const override   { return _title; }
    FrameContent* getContent()              { return _content; }
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
    
    void markForUpdate() override                               { _outdated = true; }
    void setContent(std::string content) override               { _str = content; }
    void setLinebreaks(std::vector<gint> linebreaks) override   { _linebreaks = linebreaks; }

    std::string getContent() const override                     { return _str; }
    gint getWidth() const override                              { return _width; }

private:
    bool _outdated = true;
    Frame* _frame = nullptr;
    Formatter* _formatter;
    std::string _str;
    gint _width;
    gint _height;
    gint _padding = 1;
    std::vector<gint> _linebreaks;

    // Temporary variables for inner methods
    EngineGFX* __gfx = nullptr;
    std::string __str_line;
    gint __str_lines = 0;
    gint __str_begin = 0;
    gint __str_length = 0;

    void refresh();
    void print();
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
    virtual Frame* createFrame(std::string title, Point2D extent, Point2D topleft);
    virtual Frame* createFrame(std::string title, Point2D extent);
    virtual Frame* createFrame(std::string title);

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

class FrameBuilderText : FrameBuilder {
public:
    FrameBuilderText(EngineGFX* engine) : FrameBuilder(engine) {}
    Frame* createFrame(std::string title, std::string text, Point2D extent, Point2D topleft);
    Frame* createFrame(std::string title, std::string text, Point2D extent);
    Frame* createFrame(std::string title, std::string text);

};



class Menu {
public:
    Menu();
    ~Menu();

    void addItem(MenuItem* item);
    void draw(Frame* frame);
    void execute();
    void setSelection(gint s) ;
    gint getSelection();
    std::vector<MenuItem*> getItems();

private:
    std::vector<MenuItem*> _items;
    gint _selection = 0;

};

class MenuItem {
public:
    MenuItem(std::string name, Command* command) : _name(name), _command(command) {}
    void execute()          { _command->execute(); }
    std::string toString()  { return _name; }

private:
    std::string _name;
    Command* _command;

};


} // End of namespace gGfx
} // End of namespace glib