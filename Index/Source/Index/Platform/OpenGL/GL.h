#pragma once

#ifdef INDEX_PLATFORM_MOBILE
#ifdef INDEX_PLATFORM_IOS
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#elif INDEX_PLATFORM_ANDROID
#include <GLES3/gl3.h>
#endif

#else
#ifdef INDEX_PLATFORM_WINDOWS
#include <glad/glad.h>
#elif INDEX_PLATFORM_LINUX
#include <glad/glad.h>
#elif INDEX_PLATFORM_MACOS
#include <glad/glad.h>
#include <OpenGL/gl.h>
#endif

#endif
