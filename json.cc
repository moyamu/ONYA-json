////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:json - JSON parser
// The parser implementation

#include "json.h"

#if PMU_ == 1
#include "_pmu.h"
#else
#define PMU(x)
#endif

#include <ctype.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace Json;

////////////////////////////////////////////////////////////////////////////////////////////////////

static const size_t BLOCK_SIZE = 1024;
static const unsigned ALIGNMENT  = 8;
#define ROUND_UP(n) (((n) + ALIGNMENT - 1) / ALIGNMENT * ALIGNMENT)

#define IS_SPACE(c)\
   (((unsigned char)(c) <= 0x20) && ((c) == '\x20' || (c) == '\x9' || (c) == '\xD' || (c) == '\xA'))
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')
#define IS_LCHEX(c) ((c) >= 'a' && (c) <= 'f')
#define IS_UCHEX(c) ((c) >= 'A' && (c) <= 'F')

////////////////////////////////////////////////////////////////////////////////////////////////////

static unsigned int parseHex4(char *c)
{
   unsigned value = 0;

   for (int i = 4; i > 0; --i) {
      if (IS_DIGIT(*c))      value = value * 16 + *c - '0';
      else if (IS_LCHEX(*c)) value = value * 16 + *c - 'a' + 10;
      else if (IS_UCHEX(*c)) value = value * 16 + *c - 'A' + 10;
      else return 0xFFFFFFFF;
      ++c;
   }

   return value;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

typedef struct {
   Value* obj;
   void *tail;  // where to append the next child
} StackEntry;

static inline void appendValue(StackEntry *tos, Value *child)
{
   child->next_ = 0;
   *((Value**)tos->tail) = child;
   tos->tail = &child->next_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// Key used for array elements.
static char ANONYMOUS_NULL[] = {JNULL,0};
static char ANONYMOUS_OBJECT[] = {JOBJECT,0};
static char ANONYMOUS_ARRAY[] = {JARRAY,0};
static char ANONYMOUS_STRING[] = {JSTRING,0};
static char ANONYMOUS_NUMBER[] = {JNUMBER,0};
static char ANONYMOUS_BOOL[] = {JBOOL,0};

static char BOOL_TRUE[] = "true";
static char BOOL_FALSE[] = "false";
static char NULL_VALUE[] = "null";
static const Value CONST_NULL = {ANONYMOUS_NULL + 1,0,0};

#define FAIL(pos, msg) \
   *errorPosition = pos; \
   *errorMessage = msg;\
   return 0

////////////////////////////////////////////////////////////////////////////////////////////////////

#define MAX_DEPTH 50

#define SKIP_WS() while (IS_SPACE(*s)) { ++s; }

#define T_CLOSE 0x01            // ']' or '}'
#define T_COMMA 0x02
#define T_SIMPLE 0x04
#define T_OPEN 0x08             // '{' or '{'
#define T_KEY  0x10

#define EXPECT(x) if (!(allowed & (x))) { FAIL(s,"illegal token (" #x ")"); }
#define IN_OBJECT() ((Type)stack[tos].obj->name_[-1] == JOBJECT)

#define SET_KEY_TYPE(t) \
         if (key) { key[-1] = J##t; object->name_ = key; } \
         else { object->name_ = ANONYMOUS_##t + 1; }

Value *Tree::parseInternal(char *source, const char **errorPosition, const char **errorMessage)
{
   StackEntry stack[MAX_DEPTH];
   int tos = -1;
   Value* root = 0;
   char *key = 0;
   char *s = source;
   unsigned int allowed = T_OPEN;
   char *nullpp = 0;
   char *nullp = 0;

   while (true) {
      if (nullp) {
         *nullp = 0;
      }
      nullp = nullpp;
      nullpp = 0;

      while (IS_SPACE(*s)) {
         ++s;
      }
      if (*s == 0) break;

      Value *object = 0;

      if (*s == '"') {
         EXPECT(T_SIMPLE | T_KEY);
         ++s;
         char *begin = s;
         char *wp = s;
         while (*s) {
            if ((unsigned char)*s < 0x20) {
               FAIL(s, "control character in string");
            } else if (*s == '\\') {
               switch (s[1]) {
                  case '"':  *wp = '"'; break;
                  case '\\': *wp = '\\'; break;
                  case '/':  *wp = '/'; break;
                  case 'b':  *wp = '\b'; break;
                  case 'f':  *wp = '\f'; break;
                  case 'n':  *wp = '\n'; break;
                  case 'r':  *wp = '\r'; break;
                  case 't':  *wp = '\t'; break;
                  case 'u':
                             {
                                unsigned int cp = parseHex4(s+2);
                                if (cp >= 0xD800 && cp < 0xDC00) {
                                   // handle surrogates
                                   s += 6;
                                   if (*s != '\\' || s[1] != 'u') {
                                      FAIL(s, "unrecognized escape sequence");
                                   }
                                   unsigned cp2 = parseHex4(s+2);
                                   if (cp2 < 0xDC00 || cp2 >= 0xE000)  {
                                      FAIL(s, "unrecognized escape sequence");
                                   }
                                   cp = ((cp & 0x3FF) << 10) | (cp2 & 0x3FF) | 0x10000;
                                }
                                if (cp <= 0x7F) {
                                   *wp = cp;
                                } else if (cp <= 0x7FF) {
                                   *wp++ = 0xC0 | (cp >> 6);
                                   *wp =   0x80 | (cp & 0x3F);
                                } else if (cp <= 0xFFFF) {
                                   *wp++ = 0xE0 |  (cp >> 12);
                                   *wp++ = 0x80 | ((cp >> 6) & 0x3F);
                                   *wp =   0x80 |  (cp & 0x3F);
                                } else if (cp <= 0x1FFFFF) {
                                   *wp++ = 0xF0 |  (cp >> 18);
                                   *wp++ = 0x80 | ((cp >> 12) & 0x3F);
                                   *wp++ = 0x80 | ((cp >>  6) & 0x3F);
                                   *wp =   0x80 |  (cp & 0x3F);
                                } else {
                                   FAIL(s, "unrecognized escape sequence");
                                }
                                s += 4;
                             }
                             break;
                  default:
                             FAIL(s, "unrecognized escape sequence");
               }
               ++wp;
               s += 2;
            } else if (*s == '"') {
               *wp = 0;
               ++s;
               break;
            } else {
               *wp++ = *s++;
            }
         }

         if (allowed & T_KEY) {
            key = begin;
            SKIP_WS();
            if (*s != ':') {
               FAIL(s, "missing ':'");
            }
            ++s;
            allowed = T_SIMPLE | T_OPEN;
         } else {
            object = (Value *)malloc(sizeof(Value));
            object->value_ = begin;
            SET_KEY_TYPE(STRING);
         }
      } else if (IS_DIGIT(*s) || *s == '-') {
         EXPECT(T_SIMPLE);
         object = (Value *)malloc(sizeof(Value));
         object->value_ = s;
         SET_KEY_TYPE(NUMBER);
         if (*s == '-') { ++s; }
         if (*s == '0' && IS_DIGIT(s[1])) {
            FAIL(object->value_, "leading 0 in number");
         }
         if (!IS_DIGIT(*s) && (*s != '.')) {
            FAIL(object->value_, "missing digit after '-'");
         }
         do {
            ++s;
         } while (IS_DIGIT(*s));
         if (*s == '.') {
            ++s;
         }
         while (IS_DIGIT(*s)) {
            ++s;
         }
         if ((*s == 'e') || (*s == 'E')) {
            ++s;
            if ((*s == '+') || (*s == '-')) { ++s; }
            if (!IS_DIGIT(*s)) {
               FAIL(object->value_, "missing digit in exponent");
            }
            do {
               ++s;
            } while (IS_DIGIT(*s));
         }
      } else if (s[0] == 'n' && s[1] == 'u' && s[2] == 'l' && s[3] == 'l') {
         EXPECT(T_SIMPLE);
         object = (Value *)malloc(sizeof(Value));
         object->value_ = NULL_VALUE;
         SET_KEY_TYPE(NULL);
         s += 4;
      } else if (s[0] == 't' && s[1] == 'r' && s[2] == 'u' && s[3] == 'e') {
         EXPECT(T_SIMPLE);
         object = (Value *)malloc(sizeof(Value));
         object->value_ = BOOL_TRUE;
         SET_KEY_TYPE(BOOL);
         s += 4;
      } else if (s[0] == 'f' && s[1] == 'a' && s[2] == 'l' && s[3] == 's' && s[4] == 'e') {
         EXPECT(T_SIMPLE);
         object = (Value *)malloc(sizeof(Value));
         object->value_ = BOOL_FALSE;
         SET_KEY_TYPE(BOOL);
         s += 5;
      } else if (*s == '{' || *s == '[') {
         EXPECT(T_OPEN);
         if (tos >= MAX_DEPTH - 1) {
            FAIL(s, "JSON nesting too deep");
         }
         Value *object = (Value *)malloc(sizeof(Value));
         object->value_ = 0;
         if (*s == '{') {
            allowed = T_CLOSE | T_KEY;
            SET_KEY_TYPE(OBJECT);
         } else {
            allowed = T_CLOSE | T_OPEN | T_SIMPLE;
            SET_KEY_TYPE(ARRAY);
         }
         ++s;
         // push on stack and set root
         if (tos < 0) {
            PMU(ASSERT(root == 0));
            root = object;
         } else {
            appendValue(stack + tos, object);
         }
         ++tos;
         stack[tos].obj = object;
         stack[tos].tail = &object->value_;
         key = 0;
      } else if (*s == '}' || *s == ']') {
         EXPECT(T_CLOSE);
         PMU(ASSERT(tos >= 0));
         if (stack[tos].obj->type() != ((*s == '}') ? JOBJECT : JARRAY)) {
            FAIL(s, "bracket/brace mismatch");
         }
         ++s;     // skip ']' or '}'
         --tos;   // pop from stack

         SKIP_WS();
         if (tos < 0) {
            if (*s != 0) {
               FAIL(s, "text after root element");
            }
         } else {
            if (*s == ',') {
               ++s;
               allowed = IN_OBJECT() ? T_KEY : T_SIMPLE | T_OPEN;
            } else {
               allowed = T_CLOSE;
            }
         }
         key = 0;
      } else if (*s == '/' && s[1] == '/') {
         s += 2;
         while (*s != 0 && *s != '\n') ++s;
      } else if (*s == '/' && s[1] == '*') {
         s += 2;
         while (*s != 0 && (s[0] != '*' || s[1] != '/')) ++s;
         if (*s == 0) {
            FAIL(s, "unterminated comment");
         }
         s += 2;
      } else {
         FAIL(s, "syntax error");
      }

      if (object != 0) {
         nullpp = s;
         appendValue(stack + tos, object);
         SKIP_WS();
         if (*s == ',') {
            ++s;
            allowed = IN_OBJECT() ? T_KEY : T_SIMPLE | T_OPEN;
         } else {
            allowed = T_CLOSE;
         }
      }

   }

   if (nullp) {
      *nullp = 0;
   }

   if (tos >= 0) {
      FAIL(s, "unmatched opening bracket/brace");
   }

   return root;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int Value::asInt() const
{
   switch (type()) {
      case JBOOL:
         return value_ == BOOL_TRUE ? 1 : 0;
      case JNUMBER:
         return atoi(value_);
      case JOBJECT:
      case JARRAY:
      case JSTRING:
      case JNULL:
         throw std::invalid_argument("illegal conversion to int");
   }
   return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const char *Value::asString() const
{
   switch (type()) {
      case JOBJECT:
         throw std::invalid_argument("illegal conversion of object to string");
      case JARRAY:
         throw std::invalid_argument("illegal conversion of array to string");
      default:
         break;
   }
   return value_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool Value::asBool() const
{
   switch (type()) {
      case JBOOL:
         return value_ == BOOL_TRUE;
      case JNULL:
         return false;
      case JNUMBER:
         for (const char *c = value_; *c && *c != 'e' && *c != 'E'; ++c) {
            if (IS_DIGIT(*c) && (*c != '0')) {
               return true;
            }
         }
         return false;
      case JOBJECT:
      case JARRAY:
      case JSTRING:
         return true;
   }
   return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

size_t Value::length() const
{
   if (value_ == 0) {
      return 0;
   }
   if (((Type)name_[-1] != JARRAY) && ((Type)name_[-1] != JOBJECT)) {
      return 0;
   }
   size_t len = 0;
   for (Value* x = (Value*) value_; x; x = x->next_) {
      ++len;
   }
   return len;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const Value* Value::children() const
{
   if (((Type)name_[-1] != JARRAY) && ((Type)name_[-1] != JOBJECT)) {
      throw std::invalid_argument("indexed access on simple type");
   }
   return (const Value*) value_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const Value& Value::get(int i) const
{
   const Value*x = children();
   while (i > 0 && x != 0) {
      --i;
      x = x->next_;
   }
   if ((x == 0) || (i != 0)) {
      throw std::invalid_argument("array index out of bounds");
   }
   return *x;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const Value& Value::get(const char *s) const
{
   if ((Type)name_[-1] != JOBJECT) {
      throw std::invalid_argument("member access on non-object");
   }
   const Value*x = (const Value*) value_;
   while (x != 0) {
      if (!strcmp(x->name_,s)) {
         return *x;
      }
      x = x->next_;
   }
   return CONST_NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Tree::parseInternal(char *source)
{
   const char *errorPosition = 0;
   const char *errorMessage = 0;

   root_ = parseInternal(source, &errorPosition, &errorMessage);
   if (root_ == 0) {
      throw SyntaxError(errorPosition - source, errorMessage);
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

char *Tree::malloc(size_t size)
{
   size = ROUND_UP(size);

   if (head_ == 0 || head_->eofs_ < (char*) head_ + sizeof(Chunk) + size) {
      // Insufficient free space, allocate a new chunk.
      const size_t chunkSize = ROUND_UP(std::max(sizeof(Chunk) + size, BLOCK_SIZE));
      Chunk *chunk = (Chunk*) ::malloc(chunkSize);
      if (chunk == 0) {
         throw std::runtime_error("OOM");
      }
      chunk->eofs_ = (char*) chunk + chunkSize;
      chunk->next_ = head_;
      head_ = chunk;
   }

   // Allocate from current chunk.
   return head_->eofs_ -= size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Tree::~Tree()
{
   while (head_) {
      Chunk *c = head_;
      head_ = head_->next_;
      ::free(c);
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Tree::Tree()
   : head_(0), root_(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Tree::Tree(char *source, ParseMode mode)
   : head_(0), root_(0)
{
   parse(source, mode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Tree::Tree(const char *source )
   : head_(0), root_(0)
{
   parse(source);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

Tree::Tree(const std::string &source)
   : head_(0), root_(0)
{
   parse(source);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Tree::parse(char *source, ParseMode mode)
{
   char *workingBuffer = source;
   if (mode == NON_DESTRUCTIVE) {
      const size_t sourceLength = strlen(source) + 1;
      workingBuffer = malloc(sourceLength);
      memcpy(workingBuffer, source, sourceLength);
   }
   parseInternal(workingBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Tree::parse(const char *source)
{
   parse((char *)source, NON_DESTRUCTIVE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void Tree::parse(const std::string& source)
{
   return parse((char*) source.c_str(), NON_DESTRUCTIVE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

const Value& Tree::root() const
{
   if (root_ == 0) {
      throw std::runtime_error("empty JSON document");
   }
   return *root_;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace {
   std::string formatMessage(size_t offset, const char *message)
   {
      char tmp[200];
      snprintf(tmp, sizeof(tmp), "JSON syntax error at position %lu: %s",
            (unsigned long) offset, message);
      return std::string(tmp);
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SyntaxError::~SyntaxError() throw()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

SyntaxError::SyntaxError(size_t offset, const char *message)
   : runtime_error(formatMessage(offset, message)),
     offset_(offset)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static void newLine(int level, void (*emit)(void *, char const *, size_t), void *usr)
{
   static const char nl[] = "\n                                                            ";
   static const int MAX_LEVEL = (sizeof(nl) - 2) / 2;
   emit(usr, nl, 1 + 2 * (level <= MAX_LEVEL ? level : MAX_LEVEL));
}

namespace Json {

   /////////////////////////////////////////////////////////////////////////////////////////////////

   // Note: this is actually a pure C function. 
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

   /////////////////////////////////////////////////////////////////////////////////////////////////

   static void emitStdio(void *file, char const *s, size_t len)
   {
      fwrite(s, 1, len, (FILE*)file);
   }

   void prettyPrint(char const *source, FILE *f)
   {
      prettyPrint(source, emitStdio, f);
   }

   /////////////////////////////////////////////////////////////////////////////////////////////////

   static void emitString(void *buffer, char const *s, size_t len)
   {
      ((std::string*)buffer)->append(s, len);
   }

   void prettyPrint(char const *source, std::string &buffer)
   {
      prettyPrint(source, emitString, &buffer);
   }

}

// vim:et:sw=3
