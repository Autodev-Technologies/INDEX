#pragma once

#ifndef INDEX_PLATFORM_WINDOWS
#include <signal.h>
#endif

#include <stdint.h>

#ifdef INDEX_PLATFORM_WINDOWS

#define MEM_ALIGNMENT 16
#define MEM_ALIGN __declspec(align(MEM_ALIGNMENT))

#else

#define MEM_ALIGNMENT 16
#define MEM_ALIGN __attribute__((aligned(MEM_ALIGNMENT)))

#endif

#ifdef INDEX_PLATFORM_WINDOWS
#pragma warning(disable : 4251)
#ifdef INDEX_DYNAMIC
#ifdef INDEX_ENGINE
#define INDEX_EXPORT __declspec(dllexport)
#else
#define INDEX_EXPORT __declspec(dllimport)
#endif
#else
#define INDEX_EXPORT
#endif
#define INDEX_HIDDEN
#else
#define INDEX_EXPORT __attribute__((visibility("default")))
#define INDEX_HIDDEN __attribute__((visibility("hidden")))
#endif

#define BIT(x) (1 << x)

#define NUMBONES 64
#define INPUT_BUF_SIZE 128

#define SAFE_DELETE(mem) \
    {                    \
        if(mem)          \
        {                \
            delete mem;  \
            mem = NULL;  \
        }                \
    }
#define SAFE_UNLOAD(mem, ...)         \
    {                                 \
        if(mem)                       \
        {                             \
            mem->Unload(__VA_ARGS__); \
            delete mem;               \
            mem = NULL;               \
        }                             \
    }

#ifdef INDEX_DEBUG
#define INDEX_DEBUG_METHOD(x) x;
#define INDEX_DEBUG_METHOD_CALL(x) x;
#else
#define INDEX_DEBUG_METHOD(x) \
    x                         \
    {                         \
    }
#define INDEX_DEBUG_METHOD_CALL(x) ;
#endif

#define MAX_OBJECTS 2048

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#if INDEX_PLATFORM_WINDOWS
#define INDEX_BREAK() __debugbreak();
#else
#define INDEX_BREAK() raise(SIGTRAP);
#endif

#ifdef INDEX_DEBUG
#define INDEX_ENABLE_ASSERTS
#endif

#define HEX2CHR(m_hex) \
    ((m_hex >= '0' && m_hex <= '9') ? (m_hex - '0') : ((m_hex >= 'A' && m_hex <= 'F') ? (10 + m_hex - 'A') : ((m_hex >= 'a' && m_hex <= 'f') ? (10 + m_hex - 'a') : 0)))

#ifdef INDEX_ENABLE_ASSERTS

#define INDEX_ASSERT_NO_MESSAGE(condition)        \
    {                                             \
        if(!(condition))                          \
        {                                         \
            INDEX_LOG_ERROR("Assertion Failed!"); \
            INDEX_BREAK();                        \
        }                                         \
    }

#define INDEX_ASSERT_MESSAGE(condition, ...)     \
    {                                            \
        if(!(condition))                         \
        {                                        \
            INDEX_LOG_ERROR("Assertion Failed"); \
            INDEX_LOG_ERROR(__VA_ARGS__);        \
            INDEX_BREAK();                       \
        }                                        \
    }

#define INDEX_CLIENT_ASSERT INDEX_ASSERT_MESSAGE
#define INDEX_CORE_ASSERT INDEX_ASSERT_MESSAGE
#else
#define INDEX_CLIENT_ASSERT(...)
#define INDEX_CORE_ASSERT(...)
#define INDEX_ASSERT_NO_MESSAGE(...)
#define INDEX_ASSERT_MESSAGE(condition)
#endif

#ifdef INDEX_ENGINE
#define INDEX_ASSERT INDEX_CORE_ASSERT
#else
#define INDEX_ASSERT INDEX_CLIENT_ASSERT
#endif

#define UNIMPLEMENTED                                                     \
    {                                                                     \
        INDEX_LOG_ERROR("Unimplemented : {0} : {1}", __FILE__, __LINE__); \
        INDEX_BREAK();                                                    \
    }

#define NONCOPYABLE(type_identifier)                             \
    type_identifier(const type_identifier&)            = delete; \
    type_identifier& operator=(const type_identifier&) = delete;

#if defined(_MSC_VER)
#define DISABLE_WARNING_PUSH __pragma(warning(push))
#define DISABLE_WARNING_POP __pragma(warning(pop))
#define DISABLE_WARNING(warningNumber) __pragma(warning(disable \
                                                        : warningNumber))

#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(4100)
#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(4505)
#define DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE DISABLE_WARNING(4267)

#elif defined(__GNUC__) || defined(__clang__)
#define DO_PRAGMA(X) _Pragma(#X)
#define DISABLE_WARNING_PUSH DO_PRAGMA(GCC diagnostic push)
#define DISABLE_WARNING_POP DO_PRAGMA(GCC diagnostic pop)
#define DISABLE_WARNING(warningName) DO_PRAGMA(GCC diagnostic ignored #warningName)

#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER DISABLE_WARNING(-Wunused - parameter)
#define DISABLE_WARNING_UNREFERENCED_FUNCTION DISABLE_WARNING(-Wunused - function)
#define DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE DISABLE_WARNING(-Wconversion)

#else
#define DISABLE_WARNING_PUSH
#define DISABLE_WARNING_POP
#define DISABLE_WARNING_UNREFERENCED_FORMAL_PARAMETER
#define DISABLE_WARNING_UNREFERENCED_FUNCTION
#define DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#endif

#define INDEX_SERIALISABLE(x, version) \
    CEREAL_CLASS_VERSION(x, version);  \
    CEREAL_REGISTER_TYPE_WITH_NAME(x, #x);

// #define VMA_DEBUG_LOG INDEX_LOG_WARN
//  Vulkan Only
// #define INDEX_PROFILE_GPU_ENABLED
