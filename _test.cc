////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:test - Unit test utilities
// Implementation

#include "_test.h"
#include <stdarg.h>
#include <stdexcept>
#include <stdio.h>
#include <unistd.h>

namespace Test
{
   int nTests = 0;
   int nErrors = 0;

   namespace {
      char messageBuffer[500];
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////

   Failure::Failure(const Source &where, const char *message)
      : where_(where),
        what_(message)
   {
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////

   Failure::Failure(const Source &where, const char *message, va_list args)
      : where_(where),
        what_(messageBuffer)
   {
      vsnprintf(messageBuffer, sizeof(messageBuffer), message, args);
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////

   Test *Test::current_ = 0;
   Test *Test::head_ = 0;
   Test **Test::tail_ = &head_;

   /////////////////////////////////////////////////////////////////////////////////////////////////

   Test::Test(const char*name, void(*f)())
      : next_(0), name_(name), f_(f)
   {
      *tail_ = this;
      tail_ = &next_;
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////

   int Test::runAll()
   {
      nTests = 0;
      nErrors = 0;
      for (current_ = head_; current_ != 0; current_ = current_->next_) {
         ++nTests;
         printf("+ %s\n", current_->name_);
         try {
            current_->f_();
         }
         catch (const Failure& e) {
            printf("%s:%d:: Error: TEST %s FAILED: %s\n",
                   e.where_.file_,
                   e.where_.line_,
                   current_->name_,
                   e.what_);
            ++nErrors;
         }
         catch (const std::exception & e) {
            printf("TEST %s ERROR: %s\n", current_->name_, e.what());
            ++nErrors;
         }
      }
      return nErrors;
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int Test::fail(const Source& where, const char *message, ...)
{
   va_list args;
   va_start(args, message);
   throw Failure(where,message,args);
   va_end(args);
   return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#if _POSIX_TIMERS > 0

#include <sys/time.h>

namespace Test {
   unsigned long microTime()
   {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      return ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
   }
}

#elif defined(_sun)

namespace Test {
   unsigned long microTime()
   {
      return gethrtime() / 1000;
   }
}

#else

#include <sys/time.h>

namespace Test {
   unsigned long microTime()
   {
      struct timeval tv;
      gettimeofday(&tv,0);
      return tv.tv_sec * 1000000 + tv.tv_usec;
   }
}

#endif

// vim:et:sw=3
