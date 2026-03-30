#pragma once

// Standard library
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

// OS-specific
#if defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#elif defined(__linux__) || defined(__unix__)
#include <unistd.h>
#endif

// Third-party
#include <spdlog/fmt/ostr.h>
#include <spdlog/spdlog.h>

// Core
#include "logger.h"
