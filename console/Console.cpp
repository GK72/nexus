// **********************************************
// ** gkpro @ 2019-09-17                       **
// **                                          **
// **     Console application entry point      **
// **       --- G-Library testing ---          **
// **                                          **
// **********************************************

#include <fstream>
#include "windows.h"
#include "utility.h"
#include "io.h"

#ifdef __linux__
#include "curses.h"
#endif
//#include "libgs.h"              // DLL library

int main()
{
    SetConsoleOutputCP(65001);      // Displaying Unicode characters
    std::cout << "Hello\n";


}
