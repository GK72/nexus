// libgs.cpp : Defines the exported functions for the DLL.
//

#include <iostream>
#include "libgs.h"


// This is an example of an exported variable
LIBGS_API const char* libgs_ver = "v0.01";

// This is an example of an exported function.
LIBGS_API void libgs_welcome(void)
{
    std::cout << libgs_ver;
}

// This is the constructor of a class that has been exported.
Clibgs::Clibgs()
{
    return;
}
