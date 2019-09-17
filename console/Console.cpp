// **********************************************
// ** gkpro @ 2019-09-04                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include "gmath.h"
#include "gGfx.h"
//#include "libgs.h"              // DLL library

void ml_test();
void datatable_test();

#ifdef _MSC_BUILD

class eCon : public glib::gGfx::Engine {
public:
    eCon(int width, int height, int fontWidth, int fontHeight) :
        Engine(width, height, fontWidth, fontHeight) {}


    void init() override {}
    void update(float elapsedTime) override {
        print("Welcome!");

    }
};

#endif


int main()
{
    //eCon engine(120, 80, 10, 10);
    //eCon e(120, 30, 8, 16);
    //e.run();

    glib::gGfx::Point2D a(2,2);
    glib::gGfx::Point2D b(2,1);
    glib::gGfx::Line2D l(a,b);

    std::cout << a.x;


	return 0;
}

