// **********************************************
// ** gkpro @ 2019-09-04                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include "glib/h/gmath.h"
//#include "libgs.h"              // DLL library
#include "gGfx/gGfx.h"

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

    glib::Point2D a(2,2);
    glib::Point2D b(2,2);
    glib::Line2D l(a,b);

    std::cout << l.p << '\n';
    std::cout << l.q << '\n';

	return 0;
}

