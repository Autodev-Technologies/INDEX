#pragma once

#if INDEX_PROFILE
#ifdef INDEX_PLATFORM_WINDOWS
#define TRACY_CALLSTACK 1
#endif
#include <Tracy/Tracy.hpp>
#define INDEX_PROFILE_SCOPE(name) ZoneScopedN(name)
#define INDEX_PROFILE_FUNCTION() ZoneScoped
#define INDEX_PROFILE_FRAMEMARKER() FrameMark
#define INDEX_PROFILE_LOCK(type, var, name) TracyLockableN(type, var, name)
#define INDEX_PROFILE_LOCKMARKER(var) LockMark(var)
#define INDEX_PROFILE_SETTHREADNAME(name) tracy::SetThreadName(name)
#else
#define INDEX_PROFILE_SCOPE(name)
#define INDEX_PROFILE_FUNCTION()
#define INDEX_PROFILE_FRAMEMARKER()
#define INDEX_PROFILE_LOCK(type, var, name) type var
#define INDEX_PROFILE_LOCKMARKER(var)
#define INDEX_PROFILE_SETTHREADNAME(name)

#endif
