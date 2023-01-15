#pragma once

#ifdef _WIN32
#include <Windows.h>
#include <winternl.h>
#include <TlHelp32.h>
#include <psapi.h>
#endif

#include <conio.h>

#include <iostream>
#include <map>
#include <list>
#include <thread>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <filesystem>

#include "utils/string_utils.h"