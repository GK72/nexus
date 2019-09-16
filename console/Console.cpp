// **********************************************
// ** gkpro @ 2019-09-16                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include "glib/h/gmath.h"
#include "gGfx/gGfx.h"
//#include "libgs.h"              // DLL library

void ml_test();
void datatable_test();

class eCon : public glib::gGfx::Engine {
public:
    eCon(int width, int height, int fontWidth, int fontHeight) :
        Engine(width, height, fontWidth, fontHeight) {}


    void init() override {}
    void update(float elapsedTime) override {
        print("Welcome!");

    }
};


int main()
{
    //eCon engine(120, 80, 10, 10);
    eCon e(120, 30, 8, 16);
    //e.run();

    glib::gGfx::Point2D a(2,2);
    glib::gGfx::Point2D b(2,4);
    glib::gGfx::Point2D c(2,5);
    glib::gGfx::Line2D l(a,b);
    glib::gGfx::Triangle2D tri(a, b, c);

    std::cout << "Line length: " + std::to_string(l.length()) << '\n';
    std::cout << tri.toString();
    try
    {
        //tri.draw();
        tri.getArea();
    }
    catch (const std::exception& e)
    {
        std::cout << '\n' << e.what();
    }


	return 0;
}

