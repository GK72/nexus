#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <math.h>
#include <mutex>
#include <string>
#include <thread>

#ifdef _WIN32
#include <Windows.h>
#elif __linux__
#include "curses.h"
#endif