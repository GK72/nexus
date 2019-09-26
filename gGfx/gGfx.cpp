// **********************************************
// ** gkpro @ 2019-09-16                       **
// **                                          **
// **           ---  G-Library  ---            **
// **      Graphics library implementation     **
// **                                          **
// **********************************************

#include <math.h>
#include "gGfx.h"

namespace glib {
namespace gGfx {

#ifdef _MSC_BUILD

std::atomic<bool> EngineConW::atomActive = false;

EngineConW::EngineConW(int width, int height, int fontWidth, int fontHeight)
{
    GetConsoleMode(hConsoleIn, &hConsoleOriginalIn);

    hConsoleIn = GetStdHandle(STD_INPUT_HANDLE);
    hConsoleOut = GetStdHandle(STD_OUTPUT_HANDLE);

    isConsoleInFocus = true;
    consoleCursor.bVisible = false;
    consoleCursor.dwSize = 1;
    screenWidth = width;
    screenHeight = height;
    pixels = screenWidth * screenHeight;
    windowRect = { 0, 0, screenWidth - 1, screenHeight - 1 };

    SetConsoleScreenBufferSize(hConsoleOut, COORD({ screenWidth, screenHeight }));
    SetConsoleActiveScreenBuffer(hConsoleOut);
    SetConsoleCursorInfo(hConsoleOut, &consoleCursor);

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = fontWidth;
    cfi.dwFontSize.Y = fontHeight;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hConsoleOut, false, &cfi);

    SetConsoleWindowInfo(hConsoleOut, TRUE, &windowRect);
    SetConsoleMode(hConsoleIn, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT);

    screenBuffer = new CHAR_INFO[pixels];
    memset(screenBuffer, 0, sizeof(CHAR_INFO) * pixels);

}


EngineConW::~EngineConW()
{
    delete[] screenBuffer;
    SetConsoleMode(hConsoleIn, hConsoleOriginalIn);
}

void EngineConW::run()
{
    atomActive = true;
    std::thread t(&EngineConW::MainThread, this);
    t.join();
}

void EngineConW::MainThread()
{
    init();

    auto t1 = std::chrono::system_clock::now();
    auto t2 = std::chrono::system_clock::now();

    while (atomActive)
    {
        //std::unique_lock<std::mutex> lk(mx);
        //cv.wait(lk, []{ return ; });

        // Time handling
        t2 = std::chrono::system_clock::now();
        std::chrono::duration<float> diff = t2 - t1;
        t1 = t2;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        float elapsedTime = diff.count();

        inputHandlingKeyboard();
        eventHandlingConsole();
        inputHandlingMouse();

        // Refresh screen
        memset(screenBuffer, 0, sizeof(CHAR_INFO) * pixels);
        update(elapsedTime);

        WriteConsoleOutput(hConsoleOut, screenBuffer, { screenWidth, screenHeight }, { 0, 0 }, &windowRect);
    }
}

void EngineConW::eventHandlingConsole()
{
    INPUT_RECORD inBuf[32];
    DWORD events = 0;
    GetNumberOfConsoleInputEvents(hConsoleIn, &events);
    if (events > 0) ReadConsoleInput(hConsoleIn, inBuf, events, &events);

    for (DWORD i = 0; i < events; ++i)
    {
        switch (inBuf[i].EventType)
        {
        case FOCUS_EVENT:
            isConsoleInFocus = inBuf[i].Event.FocusEvent.bSetFocus;
            break;

        case MOUSE_EVENT: {
            switch (inBuf[i].Event.MouseEvent.dwEventFlags)
            {
            case MOUSE_MOVED:
                mousePosX = inBuf[i].Event.MouseEvent.dwMousePosition.X;
                mousePosY = inBuf[i].Event.MouseEvent.dwMousePosition.Y;
                break;
            case 0:
                for (int m = 0; m < MOUSE_BUTTONS; ++m)
                    inputMouseStateNew[m] = (inBuf[i].Event.MouseEvent.dwButtonState & (1 << m)) > 0;
                break;
            default:
                break;
            }
            break;
        }
        default:
            break;
        }
    }
}
void EngineConW::inputHandlingKeyboard()
{
    for (int i = 0; i < KEY_NUMBERS; ++i)
    {
        inputKeyStateNew[i] = GetAsyncKeyState(i);
        inputKey[i].isPressed = false;
        inputKey[i].isReleased = false;

        if (inputKeyStateNew[i] != inputKeyStateOld[i])
        {
            // If the most significant bit is set, the key is down,
            // and if the least significant bit is set, the key was pressed
            // after the previous call to GetAsyncKeyState
            if (inputKeyStateNew[i] & 0x8000) {
                inputKey[i].isPressed = !inputKey[i].isHeld;
                inputKey[i].isHeld = true;
            }
            else {
                inputKey[i].isReleased = true;
                inputKey[i].isHeld = false;
            }
        }

        inputKeyStateOld[i] = inputKeyStateNew[i];
    }
}
void EngineConW::inputHandlingMouse()
{
    for (int m = 0; m < MOUSE_BUTTONS; ++m)
    {
        inputMouseButtons[m].isPressed = false;
        inputMouseButtons[m].isReleased = false;

        if (inputMouseStateNew[m] != inputMouseStateOld[m]) {
            inputMouseButtons[m].isPressed = true;
            inputMouseButtons[m].isHeld = true;
        }
        else {
            inputMouseButtons[m].isReleased = true;
            inputMouseButtons[m].isHeld = false;
        }
        inputMouseStateOld[m] = inputMouseStateNew[m];
    }
}
void EngineConW::draw(const Point2D& p, short color) const              { draw((int)p.x, (int)p.y, color); }
void EngineConW::draw(int x, int y, short color) const				    { draw(x, y, color, PIXEL_SOLID); }
void EngineConW::draw(int x, int y, short color, wchar_t ch) const
{
    screenBuffer[y * screenWidth + x].Char.UnicodeChar = ch;
    screenBuffer[y * screenWidth + x].Attributes = color;
}

void EngineConW::draw(const Sprite& sprite) const
{
    // TODO: Implement sprite draw method
    throw std::exception("Not yet implemented");
}

void EngineConW::printChar(const char ch)
{
    draw(curPosX, curPosY, FG_WHITE, ch);
    if (++curPosX >= screenWidth) {
        curPosX = 0;
        ++curPosY;
    }
}

void EngineConW::print(const char* ch)
{
    int i = 0;
    while (ch[i] != '\0') {
        printChar(ch[i]);
        ++i;
    }
}

// ************************************************************************** //

void Sprite::init(gint w, gint h)
{
    width = w;
    height = h;
    glyph = new wchar_t[w * h];
    colour = new short[w * h];

    for (int i = 0; i < w * h; ++i)
    {
        glyph[i] = ' ';
        colour[i] = COLOUR::FG_BLACK;
    }
}

void Sprite::setColour(COLOUR c)
{
    for (int i = 0; i < width * height; ++i) {
        colour[i] = c;
    }
}

void Sprite::draw(const EngineConW& gfx, int x, int y) const
{
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            gfx.draw(x + i, y + j, colour[j * width + i]);
        }
    }

}

#elif defined(__linux__)

void EngineCurses::draw(int x, int y, short color, wchar_t ch)
{
    move(curPosX, curPosY);
    delch();
    insch(ch);
}

void EngineCurses::run()
{
    init();

    auto ts = std::chrono::system_clock::now();
    auto t1 = std::chrono::system_clock::now();
    auto t2 = std::chrono::system_clock::now();

    while (engineActive)
    {
        // Time handling
        t2 = std::chrono::system_clock::now();
        std::chrono::duration<float> diff = t2 - t1;
        std::chrono::duration<float> diff_start = t2 - ts;

        t1 = t2;
        float elapsedTime = diff.count();
        float totalTime = diff_start.count();

        if (totalTime - (int)totalTime >= refreshRate) {
            update(totalTime);
        }
    }

    endwin();           // Curses: restore original window and leave
}

void EngineCurses::init()
{
    wnd = initscr();
    cbreak();           // Curses: no waiting for Enter key
    noecho();           // Curses: no echoing
    clear();            // Curses: clear screen and set cursor pos to (0, 0)

    cols = getmaxx(wnd);
    rows = getmaxy(wnd);
    curPosX = 0;
    curPosY = 0;

    engineActive = true;
}

void EngineCurses::update(float elapsedTime)
{
    //ch = getch();
    // if (ch == 'q') {
    //     engineActive = false;
    // }

    //draw(curPosX, curPosY, COLOR_WHITE, ch);
    print(std::to_string(elapsedTime));
    refresh();
}

void EngineCurses::print(const std::string& str)
{
    for (const auto& s : str) {
        draw(curPosX, curPosY, COLOR_WHITE, s);
        ++curPosY;
    }
    curPosY = 0;
}

#endif

// ************************************************************************** //
//                                 Geometry                                   //
// ************************************************************************** //

Point2D& Point2D::operator+=(const Point2D& rhs)
{
    x += rhs.x;
    y += rhs.y;
    return *this;
}

bool operator==(const Point2D& lhs, const Point2D& rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator!= (const Point2D& lhs, const Point2D& rhs) {
    return !(lhs == rhs);
}

Point2D operator+(const Point2D & lhs, const Point2D & rhs)
{
    return Point2D(lhs.x + rhs.x, lhs.y + rhs.y);
}

std::ostream& operator<< (std::ostream& out, const Point2D& rhs)
{
    std::cout << "x: " << rhs.x << '\n' << "y: " << rhs.y << '\n';
    return out;
}

Line2D::Line2D(Point2D _p, Point2D _q)
{
    if (_p != _q) {
        p = _p;
        q = _q;
    }
    else {
        throw InvalidLineException(this);
    }
}

float Line2D::length()
{
    return std::sqrt(std::pow(p.x - q.x, 2) + std::pow(p.y - q.y, 2));
}

std::string Triangle2D::toString()
{
    return std::string(
        "Point A: " + std::to_string(a.x) + ws + std::to_string(a.y) + '\n' +
        "Point B: " + std::to_string(b.x) + ws + std::to_string(b.y) + '\n' +
        "Point C: " + std::to_string(c.x) + ws + std::to_string(c.y)
    );
}

void Triangle2D::draw()
{
    _g_NIE("draw");
}

float Triangle2D::getArea()
{
    _g_uNIE;
}

float Triangle2D::getPerimeter()
{
    _g_uNIE;
}

std::string Circle::toString()
{
    return std::string();
}

void Circle::draw()
{
    _g_uNIE;
}

float Circle::getArea()
{
    _g_uNIE;
}

float Circle::getPerimeter()
{
    _g_uNIE;
}

} // End of namespace gGfx
} // End of namespace glib