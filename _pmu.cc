////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:PMU - Package maintainer utilities
// Implementation

#include "_pmu.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef NDEBUG

int pmuFail(const char *file, int line, const char *msg, ...)
{
   char tmp[200];
   int pos = snprintf(tmp,sizeof(tmp) - 1,"%s:%d::",file,line);
   if (pos < 0) { pos = 0; }

   va_list args;
   va_start(args, msg);
   int n2 = vsnprintf(tmp + pos,sizeof(tmp) - 1 - pos,msg,args);
   if (n2 < 0) {n2 = 0; }
   va_end(args);
   pos += n2;
   tmp[pos] = '\n';
   write(fileno(stderr),tmp,pos + 1);
   exit(2);
   return 1;
}


#endif

// vim:et:sw=3
