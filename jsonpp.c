////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:json - JSON parser
// JSON pretty printing command line utility.

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// copied from json.cc

#define IS_SPACE(c)\
   (((unsigned char)(c) <= 0x20) && ((c) == '\x20' || (c) == '\x9' || (c) == '\xD' || (c) == '\xA'))
#define SKIP_WS() while (IS_SPACE(*s)) { ++s; }

static void newLine(int level, void (*emit)(void *, char const *, size_t), void *usr)
{
   static const char nl[] = "\n                                                            ";
   static const int MAX_LEVEL = (sizeof(nl) - 2) / 2;
   emit(usr, nl, 1 + 2 * (level <= MAX_LEVEL ? level : MAX_LEVEL));
}


// copied from json.cc
void prettyPrint(char const *source, void (*emit)(void *, char const *, size_t), void *usr)
{
   char const *s = source;
   int level = 0;

   while (1) {
      SKIP_WS();
      if (*s == 0) break;
      if (*s == '"') {
         const char * const bos = s;
         ++s;
         while (*s != 0 && *s != '"') {
            if (*s == '\\' && s[1] != 0) {
               s += 2;
            } else {
               ++s;
            }
         }
         if (*s == '"') ++s;
         emit(usr, bos, s - bos);
      } else if (*s == '{' || *s == '[') {
         emit(usr, s++, 1);
         SKIP_WS();
         if (*s == '}' || *s == ']') { 
            emit(usr, s++, 1);
         } else {
            ++level;
            newLine(level, emit, usr);
         }
      } else if (*s == '}' || *s == ']') {
         --level;
         newLine(level, emit, usr);
         emit(usr, s++, 1);
      } else if (*s == ',') {
         emit(usr, s++, 1);
         newLine(level, emit, usr);
      } else if (*s == ':') {
         emit(usr, ": ", 2);
         ++s;
      } else {
         const char * const bos = s;
         while (*s != 0 && !IS_SPACE(*s) && *s != ',' && *s != '}' && *s != ']') ++s;
         emit(usr, bos, s - bos);
      }
   }
   emit(usr, "\n", 1);
}

void write_to_stdout(void *usr, char const *s, size_t len)
{
   fwrite(s,1,len,stdout);
}

int main()
{
   size_t capacity = 0;
   size_t size = 0;
   char *buffer = 0;
   while (1) {
      capacity = size * 2 + 10000;
      buffer = (char*) realloc(buffer, capacity);
      const int n_read = read(0, buffer + size, capacity - size - 1);
      if (n_read <= 0) {
         break;
      }
      size += n_read;
   }
   buffer[size] = 0;
   prettyPrint(buffer, write_to_stdout, 0);
   return 0;
}

// vim:et:sw=3
