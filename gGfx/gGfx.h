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

#include <atomic>
#include <chrono>
#include <exception>
#include <string>
#include <thread>
#include <Windows.h>

#include "glib/h/gmath.h"

#define _g_NIE(msg) throw NotImplementedException(msg)
#define _g_uNIE throw NotImplementedException("Unknown Exception")

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

class NotImplementedException : public std::runtime_error {
public:
    NotImplementedException(const char* msg) : std::exception(msg) {}
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

#ifdef _MSC_BUILD
class Engine
{
public:
    Engine(int width, int height, int fontWidth, int fontHeight);
    virtual ~Engine();

    void run();
    void draw(const Point2D& p, short color) const;
    void draw(int x, int y, short color) const;
    void draw(int x, int y, short color, wchar_t ch) const;
    void draw(const Sprite& sprite) const;
    void printChar(const char ch);
    void print(const char *ch);

    // Getter methods

    KeyState getKey(int keyID) const { return inputKey[keyID]; }
    KeyState getMouseButton(int buttonID) const { return inputMouseButtons[buttonID]; }
    int getMouseX() const { return mousePosX; }
    int getMouseY()	const { return mousePosY; }
    bool IsFocused() const { return isConsoleInFocus; }

protected:
    virtual void init() = 0;
    virtual void update(float elapsedTime) = 0;

private:
    static std::atomic<bool> atomActive;

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
    CHAR_INFO* screenBuffer;
    SMALL_RECT windowRect;
    CONSOLE_CURSOR_INFO consoleCursor;
    bool isConsoleInFocus;

    // Private methods
    void MainThread();
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
    void draw(const Engine& gfx, int x, int y) const;

private:
    wchar_t* glyph = nullptr;
    short* colour = nullptr;
    gint width;
    gint height;

    void init(gint w, gint h);

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
