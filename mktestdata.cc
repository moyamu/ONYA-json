////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:json - JSON parser
// Test data generator

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int MAX_DEPTH = 10;
int depth = 0;
int size[MAX_DEPTH];

static void generate(int level)
{
   printf("{\"level\":%d,",level);
   for (int i = 0; i < size[level]; ++i) {
      printf("\"integer%d\":12345678,",i);
      printf("\"boolean%d\":false,",i);
      printf("\"string%d\":\"value\",",i);
   }
   if (level + 1 < depth) {
      const char *sep = "";
      for (int i = 0; i < size[level]; ++i) {
         printf(sep);
         sep = ",";
         printf("\"object%d\":",i);
         generate(level + 1);
      }
      printf(",");

      printf("\"array\":[");
      if (level + 1 < depth) {
         const char *sep = "";
         for (int i = 0; i < size[level]; ++i) {
            printf(sep);
            sep = ",";
            generate(level + 1);
         }
      }
      printf("],");
   }
   printf("\"null\":null");
   printf("}");
}


int main(int argc, char *argv[])
{
   if ((argc < 2) || (argc > MAX_DEPTH + 1)) {
      fprintf(stderr,"Bad number of arguments\n");
      return 1;
   }
   depth = argc - 1;
   for (int i = 1; i < argc; ++i) {
      size[i - 1] = atoi(argv[i]);
      if (size[i - 1] <= 0) {
         fprintf(stderr,"Invalid depth \"%s\" at level %d\n", argv[i], i - 1);
         return 1;
      }
   }
   generate(0);
   return 0;
}
