#ifndef COMMON_PLATFORM_WIN_H
#define COMMON_PLATFORM_WIN_H

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN
#define NOWINRES
#define NOSERVICE
#define NOMCX
#define NOIME
#define NOMINMAX

#pragma push_macro("ARRAYSIZE")
#ifdef ARRAYSIZE
#undef ARRAYSIZE
#endif

#include <Windows.h>

#endif

#endif