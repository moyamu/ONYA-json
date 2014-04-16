////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:PMU - Package maintainer utilities
// Public interfaces

#ifndef ONYA_PMU_H_INCLUDED
#define ONYA_PMU_H_INCLUDED

#if PMU_ != 1

#define PMU(x)

#else

/// Prints a message and aborts the program.
#if (__GNUC__ >= 3)
__attribute((format(printf,3,4)))
#endif
int pmuFail(const char *file, int line, const char *msg, ...);

/// Aborts the program if an expression evaluates to false.
#define PMU_ASSERT(e) \
   ((e) || pmuFail(__FILE__,__LINE__, "assertion failed: %s", # e))

#define PMU(x) PMU_##x

#endif

#endif
