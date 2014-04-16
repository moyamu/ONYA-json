////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:TEST - Unit test utilities
// Public interfaces

#ifndef _TEST_H_INCLUDED
#define _TEST_H_INCLUDED

#include <stdarg.h>
#include <stdexcept>

#if (__GNUC__ >= 3)
   #define PRINTF_CHECK(i,k) __attribute__((format(printf,2,3)))
#else
   #define PRINTF_CHECK(i,k) __attribute__((format(printf,2,3)))
#endif

namespace Test
{
   /// Returns the real clock time ind microseconds since an unsepcified start time.
   unsigned long microTime();

   extern int nTests;
   extern int nErrors;

   /// Soure line informatino
   struct Source {
      const char *file_;
      const int line_;

      Source(const char *file, int line)
         : file_(file), line_(line)
      {
      }
   };

   class Failure : public std::exception
   {
   public:
      const Source where_;
      const char * const what_;
      Failure(const Source &where, const char *message);
      Failure(const Source &where, const char *message, va_list args);
      const char *what() const throw() { return what_; }
   };

   /// A test.
   class Test
   {
      static Test *head_;
      static Test **tail_;
      static Test *current_;
      Test *next_;
      const char *name_;
      void (*f_)();

   public:
      Test(const char*name, void(*f)());

      /// Runs all tests and returns the number of failed tests.
      static int runAll();
   };
   PRINTF_CHECK(2,3)
   int fail(const Source& where, const char *message, ...);
}

#define TEST(name) \
   static void test ## name ## _(); \
   static ::Test::Test test ## name(# name,test ## name ## _); \
   static void test ## name ## _()

#define HERE ::Test::Source(__FILE__,__LINE__)
#define ASSERT_THROWS(expr,et) \
   try { (expr); fail(HERE,"did not throw:%s",# expr); } catch (const et&) {}

#endif

// vim:et:sw=3
