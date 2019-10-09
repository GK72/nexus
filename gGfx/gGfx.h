// **********************************************
// ** gkpro @ 2019-09-16                       **
// **                                          **
// **           ---  G-Library  ---            **
// **      Graphics library implementation     **
// **                                          **
// **********************************************
// 
// Special thanks to OLC for source material and inspiration
// https://onelonecoder.com/
// https://www.youtube.com/javidx9
// https://www.github.com/onelonecoder

#pragma once

#include "pch.h"

#include "../glib/gmath.h"

namespace glib {
namespace gGfx {

struct Point2D;
class Line2D;

// ************************************************************************** //
//                               Exceptions                                   //
// ************************************************************************** //

class InvalidLineException : public std::runtime_error {
public:
    InvalidLineException(const Line2D* line) : std::runtime_error("Invalid Line") {}
};


#define KEY_NUMBERS 256		
#define MOUSE_BUTTONS 5

enum DIRECTION_2D
{
    NORTH,
    SOUTH,
    WEST,
    EAST
};

enum COLOUR
{
    FG_BLACK = 0x0000,
    FG_DARK_BLUE = 0x0001,
    FG_DARK_GREEN = 0x0002,
    FG_DARK_CYAN = 0x0003,
    FG_DARK_RED = 0x0004,
    FG_DARK_MAGENTA = 0x0005,
    FG_DARK_YELLOW = 0x0006,
    FG_GREY = 0x0007,
    FG_DARK_GREY = 0x0008,
    FG_BLUE = 0x0009,
    FG_GREEN = 0x000A,
    FG_CYAN = 0x000B,
    FG_RED = 0x000C,
    FG_MAGENTA = 0x000D,
    FG_YELLOW = 0x000E,
    FG_WHITE = 0x000F,
    BG_BLACK = 0x0000,
    BG_DARK_BLUE = 0x0010,
    BG_DARK_GREEN = 0x0020,
    BG_DARK_CYAN = 0x0030,
    BG_DARK_RED = 0x0040,
    BG_DARK_MAGENTA = 0x0050,
    BG_DARK_YELLOW = 0x0060,
    BG_GREY = 0x0070,
    BG_DARK_GREY = 0x0080,
    BG_BLUE = 0x0090,
    BG_GREEN = 0x00A0,
    BG_CYAN = 0x00B0,
    BG_RED = 0x00C0,
    BG_MAGENTA = 0x00D0,
    BG_YELLOW = 0x00E0,
    BG_WHITE = 0x00F0,
};
enum PIXEL_TYPE
{
    PIXEL_SOLID = 0x2588,
    PIXEL_THREEQUARTERS = 0x2593,
    PIXEL_HALF = 0x2592,
    PIXEL_QUARTER = 0x2591,
};

class Sprite;

struct KeyState {
    bool isPressed;
    bool isReleased;
    bool isHeld;
};

class Engine
{
public:
    virtual void init() = 0;
    virtual int  inputHandling() = 0;
    virtual void update(float elapsedTime) = 0;

};

class EngineGFX 
{
public:
    EngineGFX() {}
    EngineGFX(Engine* engine) : engine(engine) {}
    virtual void run() = 0;

    virtual void draw(int x, int y, short color, wchar_t ch) const = 0;
    virtual void draw(const Sprite& sprite) const = 0;

    virtual void print(std::string str) = 0;
    virtual void printn(std::string str) = 0;
    virtual void printn() = 0;
    virtual void printr(std::string str) = 0;

    virtual void setCurPosY(short y) = 0;
    virtual void setCurPosX(short x) = 0;

    // Getter methods

    virtual KeyState getKey(int keyID) const = 0;
    virtual KeyState getMouseButton(int buttonID) const = 0;
    virtual short getScreenWidth() const = 0;
    virtual short getScreenHeight() const = 0;
    virtual int getMouseX() const = 0;
    virtual int getMouseY()	const = 0;
    virtual bool IsFocused() const = 0;

protected:
    Engine* engine = nullptr;

};

#ifdef _MSC_BUILD
class EngineConW : public EngineGFX
{
public:
    EngineConW(Engine* engine, int width, int height, int fontWidth, int fontHeight);
    virtual ~EngineConW();

    void run();
    void draw(int x, int y, short color, wchar_t ch) const;
    void draw(const Sprite& sprite) const;

    void print(char ch);
    void print(std::string str);
    void printn(std::string str);
    void printn();
    void printr(std::string str);

    void setCurPosY(short y)                    { curPosY = y; }
    void setCurPosX(short x)                    { curPosX = x; }

    // Getter methods

    KeyState getKey(int keyID) const            { return inputKey[keyID]; }
    KeyState getMouseButton(int buttonID) const { return inputMouseButtons[buttonID]; }
    short getScreenWidth() const                { return screenWidth; }
    short getScreenHeight() const               { return screenHeight; }
    int getMouseX() const                       { return mousePosX; }
    int getMouseY()	const                       { return mousePosY; }
    bool IsFocused() const                      { return isConsoleInFocus; }

protected:
    CHAR_INFO* screenBuffer;

private:
    bool hasInputEvent = true;                  // Because the first frame needs to be rendered

    short screenWidth;
    short screenHeight;
    int pixels;

    short inputKeyStateOld[KEY_NUMBERS];
    short inputKeyStateNew[KEY_NUMBERS];
    bool  inputMouseStateOld[MOUSE_BUTTONS];
    bool  inputMouseStateNew[MOUSE_BUTTONS];
    KeyState inputKey[KEY_NUMBERS];
    KeyState inputMouseButtons[MOUSE_BUTTONS];
    int mousePosX;
    int mousePosY;

    short curPosX = 0;
    short curPosY = 0;

    HANDLE hConsoleOut;
    HANDLE hConsoleIn;
    DWORD hConsoleOriginalIn;
    SMALL_RECT windowRect;
    CONSOLE_CURSOR_INFO consoleCursor;
    bool isConsoleInFocus;

    // Private methods

    void InputThread();
    void DisplayThread();
    void eventHandlingConsole();
    void inputHandlingKeyboard();
    void inputHandlingMouse();

};

// ************************************************************************** //

class Sprite {
public:
    Sprite(int w, int h)	{ init(w, h); }
    ~Sprite()				{ delete[] glyph; delete[] colour; }

    void setColour(COLOUR colour);
    void draw(const EngineConW& gfx, int x, int y) const;

private:
    wchar_t* glyph = nullptr;
    short* colour = nullptr;
    gint width;
    gint height;

    void init(gint w, gint h);

};

#elif defined(__linux__)

class EngineCurses : public EngineGFX
{
public:
    void draw(int x, int y, short color, wchar_t ch);
    void run();

private:
    void init();
    void update(float elapsedTime);
    void print(const std::string& str);

    WINDOW* wnd;
    int cols;
    int rows;
    int curPosX;
    int curPosY;

    bool engineActive;
    char ch;

    int refreshRate = 1;
};

#endif

// ************************************************************************** //
//                                 Geometry                                   //
// ************************************************************************** //

struct Point2D
{
    float x;
    float y;

    Point2D() { x = 0, y = 0; }
    Point2D(float _x, float _y) { x = _x; y = _y; }

    friend bool operator== (const Point2D& lhs, const Point2D& rhs);
    friend bool operator!= (const Point2D& lhs, const Point2D& rhs);
    friend Point2D operator+ (const Point2D& lhs, const Point2D& rhs);
    friend std::ostream& operator<< (std::ostream& out, const Point2D& rhs);

    Point2D& operator+= (const Point2D& rhs);
};

class Line2D
{
public:
    Line2D(Point2D _p, Point2D _q);

    float length();

private:
    Point2D p;
    Point2D q;

};

class Shape {
    virtual std::string toString()  = 0;
    virtual void draw()             = 0;
    virtual float getArea()         = 0;
    virtual float getPerimeter()    = 0;
};

class Triangle2D : public Shape {
public:
    Triangle2D(Point2D _a, Point2D _b, Point2D _c) : a(_a), b(_b), c(_c) {}

    std::string toString();
    void draw();
    float getArea();
    float getPerimeter();

private:
    Point2D a;
    Point2D b;
    Point2D c;
    const char* ws = "\t";
};

class Circle : public Shape {
public:
    Circle(Point2D center, float radius) : c(center), r(radius) {}

    std::string toString();
    void draw();
    float getArea();
    float getPerimeter();

private:
    Point2D c;
    float r;
};


} // End of namespace gGfx
} // End of namespace glib