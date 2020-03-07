#ifndef DEBUG_H_
#define DEBUG_H_
void exception_report(const char *file, int line,
                      const char *func, const char *error_source);
#define EXCEPTION_REPORT(...) \
    exception_report(__FILE__, __LINE__, __func__, __VA_ARGS__)
#ifdef NDEBUG
#define ASSERT(CONDITION) (void(0))
#else
#define ASSERT(CONDITION)             \
    if (CONDITION)                    \
    {                                 \
    }                                 \
    else                              \
    {                                 \
        EXCEPTION_REPORT(#CONDITION); \
    }

#endif

#endif