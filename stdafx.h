// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// prevent windef.h from defining min and max macros, please use std::min and std::max
#define NOMINMAX

#include "targetver.h"

// windows headers are found in
// C:\Program Files (x86)\Windows Kits\10\Include\10.0.18362.0\um

//#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <winuser.h>
#include <WinDef.h>
#include <winnt.h>
#include <WindowsX.h>
#include <synchapi.h>
#include <sysinfoapi.h>
#include <strsafe.h>
#include <hidusage.h>
#include <shellapi.h>

// C RunTime Header Files
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <crtdbg.h>
#include <time.h>
#include <io.h>
#include <assert.h>
#include <profileapi.h>
#include <math.h>
#include <array>

// STL
#include <string>
#include <bitset>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <limits>
#include <algorithm>
#include <list>
#include <unordered_map>
#include <fstream>
#include <vector>
#include <iostream>

// 3rd party
//#include <GL/glew.h>
//#include <GL/wglew.h>

#define _USE_MATH_DEFINES
#include <math.h>

//#pragma warning( disable: 996 ) // = _CRT_SECURE_NO_WARNINGS.

// TODO: reference additional headers your program requires here

