#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

#ifdef _MSC_BUILD
#include <Windows.h>
#elif __linux__
#include "curses.h"
#endif