////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:json - JSON parser
// Test program

#include "_pmu.h"
#include "_test.h"
#include "json.h"
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace ApiTest {

   using Json::DESTRUCTIVE;

   void foo()
   {
      char *c = 0;
      Json::Tree(c,DESTRUCTIVE);
   }

}

using Test::Source;
using namespace Json;

static bool isNear(double a, double b, double epsilon)
{
   if (b == 0) {
      return a < epsilon && a > -epsilon;
   }

   // Reduce to the case where a>=0 and b>=0
   if (((a > 0) && (b < 0)) || ((a < 0) && (b > 0))) {
      return false;
   }
   if (b < 0) {
      a = -a;
      b = -b;
   }

   // Check if the division below would overflow
   if ((b < 1.0) && (a > b * DBL_MAX)) {
      return false;
   }

   return (1 - epsilon) < (double)(a / b) && (double)(a / b) < (1 + epsilon);
}

static void assertInt(const Source& where, const Value& e, int value, const char *expr)
{
   if (e.type() != JNUMBER) {fail(where, "%s: type is not NUMBER but %d", expr, e.type()); }
   if (e.asInt() != value) {
      fail(where, "%s: value is %d, expected %d", expr, e.asInt(), value);
   }
}

static void assertFloat(const Source& where, const Value& e, double value, const char *expr)
{
   static const double EPSILON = 1e-7;
   if (e.type() != JNUMBER) {fail(where, "%s: type is not NUMBER but %d", expr, e.type()); }
   if (!isNear(e.asDouble(), value, EPSILON)) {
      fail(where, "%s: value=%f expected=%f", expr, e.asDouble(), value);
   }
}

static void assertBool(const Source& where, const Value& e, bool value, const char *expr)
{
   if (e.type() != JBOOL) {fail(where, "%s: type is not BOOL but %d", expr, e.type()); }
   if (e.asBool() != value) {
      fail(where, "%s: value=%d expected=%d", expr, e.asInt(), value);
   }
}

static void assertString(const Source& where, const Value& e, const char *value, const char *expr)
{
   if (e.type() != JSTRING) {fail(where, "%s: type is not STRING but %d", expr, e.type()); }
   if (strcmp(e.asString(), value)) {
      fail(where, "%s: value=\"%s\" expected=\"%s\"", expr, e.asString(), value);
   }
}

static void assertNull(const Source& where, const Value& e, const char *expr)
{
   if (e.type() != JNULL) {fail(where, "%s: type is not NULL but %d", expr, e.type()); }
}

static void assertArray(const Source& where, const Value& e, size_t len, const char *expr)
{
   if (e.type() != JARRAY) {fail(where, "%s: type is not ARRAY but %d", expr, e.type()); }
   const size_t l = e.length();
   if (l != len) {fail(where,
                       "%s: array length is %u, expected %u",
                       expr,
                       (unsigned) l,
                       (unsigned)len); }
}

static void assertObject(const Source& where, const Value& e, size_t len, const char *expr)
{
   if (e.type() != JOBJECT) {fail(where, "%s: type is not ARRAY but %d", expr, e.type()); }
   const size_t l = e.length();
   if (l != len) {fail(where,
                       "%s: object size is %u, expected %u",
                       expr,
                       (unsigned) l,
                       (unsigned)len); }
}

static void assertEq(const Source& where, const char *val, const char *exp, const char *expr)
{
   if (strcmp(val,exp)) {
      fail(where, "%s: value is \"%s\", expected \"%s\"", expr, val, exp);
   }
}

#define ASSERT(e) ((e) || ::Test::fail(HERE,"FAILED: %s",# e))
#define ASSERT_EQ(e,exp) assertEq(HERE,e,exp,# e)
#define ASSERT_INT(e, v) assertInt(HERE,e,v,# e)
#define ASSERT_FLOAT(e, v)  assertFloat(HERE,e,v,# e)
#define ASSERT_BOOL(e, v) assertBool(HERE,e,v,# e)
#define ASSERT_STRING(e, v) assertString(HERE,e,v,# e)
#define ASSERT_NULL(e) assertNull(HERE,e,# e)
#define ASSERT_ARRAY(e,l) assertArray(HERE,e,l,# e)
#define ASSERT_OBJECT(e,l) assertObject(HERE,e,l,# e)

void print(Value *v, int indent = 0)
{
   const Type type = v->type();

   if (type == JOBJECT) {
      printf("{\n");
      indent += 3;
      for (Value *e = (Value *)v->value_; e; e = e->next_) {
         printf("%*s\"%s\": ",indent,"",e->name_);
         print(e,indent);
      }
      indent -= 3;
      printf("%*s}",indent,"");
   } else if (type == JARRAY) {
      printf("[\n");
      indent += 3;
      for (Value *e = (Value *)v->value_; e; e = e->next_) {
         printf("%*s",indent,"");
         print(e,indent);
      }
      indent -= 3;
      printf("%*s]",indent,"");
   } else if (type == JSTRING) {
      printf("\"%s\"",v->value_);
   } else {
      printf("%s",v->value_);
   }
   printf(v->next_ ? ",\n" : "\n");
   fflush(stdout);
}

static void assertParserError(const Test::Source &where, const char *source, size_t errorOffset)
{
   try {
      Tree tree;
      tree.parse(source);
      fail(where, "Invalid JSON successfully parsed: %s", source);
   }
   catch (const SyntaxError& e) {
      if ((errorOffset != (size_t) -1) && (e.offset_ != errorOffset)) {
         fail(where, "Offset %u, expected %u. Message:\"%s\"\n",
              (unsigned)e.offset_,
              (unsigned)errorOffset,
              e.what());
      }
   }
}

TEST(Api)
{
   const char SRC[] = "[1,{\"a\":true},3]";
   char buf[sizeof(SRC)];

   memcpy(buf,SRC,sizeof(buf));
   Tree a1(SRC);
   Tree a2(buf, NON_DESTRUCTIVE);
   Tree a3(buf, DESTRUCTIVE);

   memcpy(buf,SRC,sizeof(buf));
   Tree b1;
   b1.parse(SRC);
   Tree b2;
   b2.parse(buf,NON_DESTRUCTIVE);
   Tree b3;
   b3.parse(buf,DESTRUCTIVE);

   ASSERT(b3[2].asInt() == 3);
   ASSERT_BOOL(b3[1]["a"],true);
   ASSERT(b3.root()[2].asInt() == 3);

   for (const Value *i = b3.root().children(); i; i = i->next_) {
   }
}

#define ASSERT_PARSER_ERROR(source,errorOffset) assertParserError(HERE,source,errorOffset);

TEST(BadRoot_String)
{
   ASSERT_PARSER_ERROR("\"string\"",0);
}

TEST(BadRoot_Integer)
{
   ASSERT_PARSER_ERROR("0",0);
}

TEST(BadRoot_Null)
{
   ASSERT_PARSER_ERROR("null",0);
}

TEST(BadRoot_Boolean)
{
   ASSERT_PARSER_ERROR("true",0);
}

TEST(BadRoot_TwoArrays)
{
   ASSERT_PARSER_ERROR("[],[]",2);
}

TEST(BadRoot_TwoObjects)
{
   ASSERT_PARSER_ERROR("{},{}",2);
}

TEST(BadRoot_ObjectAndNull)
{
   ASSERT_PARSER_ERROR("{},null",2);
}

TEST(BadRoot_ObjectAndInt)
{
   ASSERT_PARSER_ERROR("{},0",2);
}

TEST(BadRoot_ArrayAndNull)
{
   ASSERT_PARSER_ERROR("{},null",2);
}

TEST(BadRoot_TrailingComma)
{
   ASSERT_PARSER_ERROR("{},",2);
}

TEST(MissingKeyNull)
{
   ASSERT_PARSER_ERROR("{null}", 1);
}

TEST(MissingKeyInt)
{
   ASSERT_PARSER_ERROR("{0}", 1);
}

TEST(MissingKeyObject)
{
   ASSERT_PARSER_ERROR("{{}}", 1);
}

TEST(IncompleteObject)
{
   ASSERT_PARSER_ERROR("{\"Key\":true,", 12);
}

TEST(IncompleteArray)
{
   ASSERT_PARSER_ERROR("[", 1);
}

TEST(BracketBraceMismatch)
{
   ASSERT_PARSER_ERROR("[1,2}", 4);
}

TEST(BraceBracketMismatch)
{
   ASSERT_PARSER_ERROR("{\"Key\":0]", 8);
}

TEST(NakedWord)
{
   ASSERT_PARSER_ERROR("[naked]", 1);
}

TEST(NakedWord2)
{
   ASSERT_PARSER_ERROR("[\\naked]", 1);
}

TEST(NakedWordAsValue)
{
   ASSERT_PARSER_ERROR("{\"Key\":naked}", 7);
}

TEST(NakedWordAsKey)
{
   ASSERT_PARSER_ERROR("{Key:0}", 1);
}

TEST(MissingColon)
{
   ASSERT_PARSER_ERROR("{\"Key\" null}", 7);
}

TEST(DoubleColon)
{
   ASSERT_PARSER_ERROR("{\"Key\":: null}", 7);
}

TEST(CommaInsteadOfColon)
{
   ASSERT_PARSER_ERROR("{\"Key\", 0}", 6);
}

TEST(ColonInsteadOfComma)
{
   ASSERT_PARSER_ERROR("[\"Key\": 0]", 6);
}

TEST(MissingComma)
{
   ASSERT_PARSER_ERROR("[0 1]", 3);
}

TEST(MissingComma2)
{
   ASSERT_PARSER_ERROR("[0 null]", 3);
}

TEST(MissingComma3)
{
   ASSERT_PARSER_ERROR("[{} {}]", 4);
}

TEST(ExtraComma)
{
   ASSERT_PARSER_ERROR("[0,1,]", 5);
}

TEST(ExtraComma2)
{
   ASSERT_PARSER_ERROR("{\"Key\":0,}", 9);
}

TEST(ExtraComma3)
{
   ASSERT_PARSER_ERROR("[,]", 1);
}

TEST(IllegalExpressionInValue)
{
   ASSERT_PARSER_ERROR("{\"Key\": 1 + 2}",10);
}

TEST(IllegalExpressionInArray)
{
   ASSERT_PARSER_ERROR("[1 + 2]",3);
}

TEST(BadNumber_Octal)
{
   ASSERT_PARSER_ERROR("[013]",1);
}

TEST(BadNumber_Hex)
{
   ASSERT_PARSER_ERROR("[0x13]",2);
}

TEST(BadNumber_LeadingDot)
{
   ASSERT_PARSER_ERROR("[.5]",1);
}

TEST(BadNumber_EmptyExponent)
{
   ASSERT_PARSER_ERROR("[0e]",1);
}

TEST(BadNumber_EmptyExponent2)
{
   ASSERT_PARSER_ERROR("[0e+]",1);
}

TEST(BadNumber_BadExponent)
{
   ASSERT_PARSER_ERROR("[0e+-1]",1);
}

TEST(BadString_HexEscape)
{
   ASSERT_PARSER_ERROR("[\"Bad \\x15\"]",6);
}

TEST(BadString_OctalEscape)
{
   ASSERT_PARSER_ERROR("[\"Bad \\015\"]",6);
}

TEST(BadString_SingleQuote)
{
   ASSERT_PARSER_ERROR("['Bad']",1);
}

TEST(BadString_LeadingTab)
{
   ASSERT_PARSER_ERROR("[\"\ttab\"]",2);
}

TEST(BadString_TrailigTab)
{
   ASSERT_PARSER_ERROR("[\"tab\t\"]",5);
}

TEST(BadString_EmbeddedTab)
{
   ASSERT_PARSER_ERROR("[\"tab\ttab\"]",5);
}

TEST(BadString_UnknownEscape)
{
   ASSERT_PARSER_ERROR("[\"\\ \"]",2);
}

TEST(BadString_LineBreak)
{
   ASSERT_PARSER_ERROR("[\"line\nbreak\"]",6);
}

TEST(BadString_LineBreak2)
{
   ASSERT_PARSER_ERROR("[\"escaped line\\\nbreak\"]",14);
}

//TEST(DuplicateAttribute)    { ASSERT_PARSER_ERROR("{\"A\":0,\"A\":0}",7); }

TEST(EmptyObject)
{
   Tree doc("{}");
   ASSERT(doc.type() == JOBJECT);
   ASSERT(doc.length() == 0);
}

TEST(EmptyArray)
{
   Tree doc("[]");
   ASSERT(doc.type() == JARRAY);
   ASSERT(doc.length() == 0);
}

TEST(EmptyObjectAndNumber)
{
   Tree doc("{\"a\":{},\"b\":1}");
   ASSERT(doc.type() == JOBJECT);
   ASSERT(doc.get("a").type() == JOBJECT);
   ASSERT_INT(doc.get("b"),1);
}

TEST(ObjArrayObj)
{
   Tree doc("{\"a\":[{},{}]}");
   ASSERT_OBJECT(doc.root(),1);
   ASSERT_ARRAY(doc.get("a"), 2);
   ASSERT_OBJECT(doc.get("a").get(0),0);
   ASSERT_OBJECT(doc.get("a").get(1),0);
}

TEST(Integers)
{
   Tree doc("[42,-42,0]");
   ASSERT_INT(doc.get(0),42);
   ASSERT_INT(doc.get(1),-42);
   ASSERT_INT(doc.get(2),0);
}

TEST(ObjectAsIntThrows)
{
   Tree doc("{}");
   ASSERT_THROWS(doc.asInt(), std::invalid_argument);
}

TEST(ArrayAsIntThrows)
{
   Tree doc("[]");
   ASSERT_THROWS(doc.asInt(), std::invalid_argument);
}

TEST(BooleanAsInt)
{
   Tree doc("[false,true]");
   ASSERT(doc.get(0).asInt() == 0);
   ASSERT(doc.get(1).asInt() == 1);
}

TEST(Floats)
{
   Tree doc(
      "[0., 0.125, -0.125, 1.5e5, -1.5E-5]");
   ASSERT_FLOAT(doc.get(0),0);
   ASSERT_FLOAT(doc.get(1),0.125);
   ASSERT_FLOAT(doc.get(2),-0.125);
   ASSERT_FLOAT(doc.get(3),1.5e5);
   ASSERT_FLOAT(doc.get(4),-1.5e-5);
}

TEST(Boolean)
{
   Tree doc("[true,false]");
   ASSERT_BOOL(doc.get(0),true);
   ASSERT_BOOL(doc.get(1),false);
}

TEST(BooleanAsString)
{
   Tree doc("[true,false]");
   ASSERT_EQ(doc.get(0).asString(),"true");
   ASSERT_EQ(doc.get(1).asString(),"false");
}

TEST(ObjectAsStringThrows)
{
   Tree doc("{}");
   ASSERT_THROWS(doc.asString(), std::invalid_argument);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(EmptyStringIsTruthy)
{
   Tree doc("[\"\"]");
   ASSERT(doc.get(0).asBool() == true);
}

TEST(EmptyObjectIsTruthy)
{
   Tree doc("{}");
   ASSERT(doc.asBool() == true);
}

TEST(EmptyArrayIsTruthy)
{
   Tree doc("[]");
   ASSERT(doc.asBool() == true);
}

TEST(NonzeroNumberIsTruthy)
{
   Tree doc("[1,0.1]");
   ASSERT(doc.get(0).asBool() == true);
   ASSERT(doc.get(1).asBool() == true);
}

TEST(ZeroNumberIsFalsy)
{
   Tree doc("[0,0.0,0.0e5]");
   ASSERT(doc.get(0).asBool() == false);
   ASSERT(doc.get(1).asBool() == false);
   ASSERT(doc.get(2).asBool() == false);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(EscapedCharacters)
{
   Tree doc(
      "[\"(\\\") (\\\\) (\\/) (\\b) (\\f) (\\n) (\\r) (\\t)\"]");
   ASSERT_STRING(doc.get(0),
                 "(\") (\\) (/) (\b) (\f) (\n) (\r) (\t)");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(TruncatedUnicode)
{
   ASSERT_PARSER_ERROR("[\"\\u123\"]",2);
}

TEST(BadUnicode)
{
   ASSERT_PARSER_ERROR("[\"\\u 123\"]",2);
}

TEST(Unicode)
{
   Tree doc(
      "[\"\\u0123\\u4567\\u89AB\\uCDEF\\uabcd\\uef4A\"]");
   ASSERT_STRING(doc.get(0),
                 "\xc4\xa3"
                 "\xe4\x95\xa7"
                 "\xe8\xa6\xab"
                 "\xec\xb7\xaf"
                 "\xea\xaf\x8d"
                 "\xee\xbd\x8a");
}

TEST(Utf16Surrogates)
{
   // U+1D11E = UTF-16: D834, DD1E = UTF-8: f0 9d 84 9e
   Tree doc(
      "[\"\\uD834\\uDD1E\"]"); ASSERT_STRING(doc.get(0), "\xf0\x9d\x84\x9e" );
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(Strings)
{
   Tree doc(
      "["
      "\"\","
      "\" \","
      "\"test\","
      "\"\\u0040\","
      "\"embedded quotes:\\\"\""
      "]");
   ASSERT_STRING(doc.get(0),"");
   ASSERT_STRING(doc.get(1)," ");
   ASSERT_STRING(doc.get(2),"test");
   ASSERT_STRING(doc.get(3),"@");
   ASSERT_STRING(doc.get(4),"embedded quotes:\"");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(Null)
{
   Tree doc("[null]");
   ASSERT_NULL(doc.get(0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(Array)
{
   Tree doc("[42]");
   ASSERT_ARRAY(doc.root(),1);
   ASSERT_INT(doc.get(0),42);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(OneByteValuesInArray)
{
   Tree doc("[0,1,2]");
   ASSERT_ARRAY(doc.root(),3);
   ASSERT_INT(doc.get(0),0);
   ASSERT_EQ(doc.get(0).asString(),"0");
   ASSERT_INT(doc.get(1),1);
   ASSERT_EQ(doc.get(1).asString(),"1");
   ASSERT_INT(doc.get(2),2);
   ASSERT_EQ(doc.get(2).asString(),"2");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(DeepArray)
{
   Tree doc(
      "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[42]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]");
   ASSERT_ARRAY(doc.root(),1);
   const Value *x = &doc.root();
   for (int i = 0; i < 45; ++i) {
      ASSERT_ARRAY(*x,1);
      x = &x->get(0);
   }
   ASSERT_INT(*x,42);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(EmptyKey)
{
   Tree doc("{\"\":\"emptyKey\"}");
   ASSERT_OBJECT(doc.root(),1);
   ASSERT_STRING(doc.get(""),"emptyKey");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TEST(UndefinedKeyReturnsJsonNull)
{
   Tree doc("{}");
   ASSERT_NULL(doc.get("x"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(SimpleTypes)
{
   Tree doc(
      "{"
      "\"null\":null,"
      "\"true\":true,"
      "\"false\":false,"
      "\"int\":123,"
      "\"float\":1.234,"
      "\"string\":\"value\""
      "}"
      );

   //print(doc->root());
   ASSERT_OBJECT(doc.root(),6);
   ASSERT_BOOL(doc.get("true"),true);
   ASSERT_BOOL(doc.get("false"),false);
   ASSERT_INT(doc.get("int"),123);
   ASSERT_FLOAT(doc.get("float"),1.234);
   ASSERT_NULL(doc.get("null"));
   ASSERT_STRING(doc.get("string"),"value");
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(ThrowOnNegativeIndex)
{
   Tree doc("[1]");
   ASSERT_THROWS(doc.get(-1), std::invalid_argument);
}

TEST(ThrowIfIndexAfterEnd)
{
   Tree doc("[1]");
   ASSERT_THROWS(doc.get(1), std::invalid_argument);
}

TEST(ThrowOnIndexedAccessOnString)
{
   Tree doc("[\"aaaa\"]");
   ASSERT_THROWS(doc.get(0).get(0), std::invalid_argument);
}

TEST(ThrowOnAttributeAccessOnArray)
{
   Tree doc("[\"aaaa\"]");
   ASSERT_THROWS(doc.get("a"), std::invalid_argument);
}

TEST(ThrowOnAttributeAccessOnString)
{
   Tree doc("[\"aaaa\"]");
   ASSERT_THROWS(doc.get(0).get("a"), std::invalid_argument);
}

TEST(ThrowOnZeroIndexWithEmptyArray)
{
   Tree doc("[]");
   ASSERT_THROWS(doc.get(0), std::invalid_argument);
}

TEST(ChildrenIsNullOnEmptyArray)
{
   Tree doc("[]");
   ASSERT(doc.root().children() == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(LengthOfSimpleTypesIsZero)
{
   Tree doc("[\"aaaa\",1,true,null]");
   ASSERT(doc.get(0).length() == 0);
   ASSERT(doc.get(1).length() == 0);
   ASSERT(doc.get(2).length() == 0);
   ASSERT(doc.get(3).length() == 0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(OneLineComments)
{
   Tree doc("[0,//42,\n1]");
   ASSERT_INT(doc.get(0), 0);
   ASSERT_INT(doc.get(1), 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(MultiLineComments)
{
   Tree doc("[0,/*42,\n42,*/1]");
   ASSERT_INT(doc.get(0), 0);
   ASSERT_INT(doc.get(1), 1);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(FormatterEmptyObj)
{
   char json[] =
      "{"
         "\"empty\":{\n}"
      "}";
   std::string EXPECTED = "{\n  \"empty\": {}\n}\n";
   std::string formatted;
   prettyPrint(json, formatted);
   ASSERT(formatted == EXPECTED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(FormatterObj)
{
   char json[] =
      "{"
         "\"one\":1,"
         "\"null\":null"
      "}";
   std::string EXPECTED = "{\n  \"one\": 1,\n  \"null\": null\n}\n";
   std::string formatted;
   prettyPrint(json, formatted);
   ASSERT(formatted == EXPECTED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(FormatterEmptyArray)
{
   char json[] = "[[],[]]";
   std::string EXPECTED = "[\n  [],\n  []\n]\n";
   std::string formatted;
   prettyPrint(json, formatted);
   ASSERT(formatted == EXPECTED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(FormatterArray)
{
   char json[] = "[1,2]";
   std::string EXPECTED = "[\n  1,\n  2\n]\n";
   std::string formatted;
   prettyPrint(json, formatted);
   ASSERT(formatted == EXPECTED);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

TEST(FormatterNested)
{
   char json[] =
      "{"
         "\"array\":[1,{\"one\":1}],"
         "\"obj\":{\"obj\":{}}"
      "}";
   std::string EXPECTED = 
      "{\n"
      "  \"array\": [\n"
      "    1,\n"
      "    {\n"
      "      \"one\": 1\n"
      "    }\n"
      "  ],\n"
      "  \"obj\": {\n"
      "    \"obj\": {}\n"
      "  }\n"
      "}\n";

   std::string formatted;
   prettyPrint(json, formatted);
   ASSERT(formatted == EXPECTED);
}


////////////////////////////////////////////////////////////////////////////////////////////////////

char *readFile(const char *fn)
{
   int f = open(fn,O_RDONLY);
   if (f < 0) {
      fail(HERE,"Cannot open %s: %s", fn, strerror(errno));
   }
   struct stat sb;
   fstat(f,&sb);
   char *buf = (char*) malloc(sb.st_size + 1);
   if (read(f,buf,sb.st_size) != sb.st_size) {
      fail(HERE,"Cannot read %s: %s", fn, strerror(errno));
   }
   buf[sb.st_size] = 0;
   close(f);
   return buf;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

static void performanceTest(const char *fn)
{
   const char * const data = readFile(fn);
   const unsigned long nBytes = strlen(data);

   for (int i = 0; i < 10; ++i) {
      char *c = (char*) malloc(nBytes + 1);
      memcpy(c,data,nBytes + 1);
      unsigned long t = Test::microTime();
      Tree doc(c,DESTRUCTIVE);
      t = Test::microTime() - t;
      printf("%-20s: %10ldBytes, %10.6fs, %7.1fMB/s\n", fn, nBytes,t/1e6,nBytes * 1.0 / t);
      free(c);
   }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv)
{
   printf("sizeof(Value)=%u\n",(unsigned)sizeof(Value));
   if (argc > 1) {
      for (int i = 1; i < argc; ++i) {
         performanceTest(argv[i]);
      }
      return 0;
   }

   Test::Test::runAll();
   return Test::nErrors == 0 ? 0 : 1;
}

// vim:et:sw=3
