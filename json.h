////////////////////////////////////////////////////////////////////////////////////////////////////
// I, the creator of this work, hereby release it into the public domain. This applies worldwide.
// In case this is not legally possible: I grant anyone the right to use this work for any purpose,
// without any conditions, unless such conditions are required by law.
////////////////////////////////////////////////////////////////////////////////////////////////////

// ONYA:json - JSON parser
// Public interfaces

#ifndef JSON_H
#define JSON_H

#include <stdexcept>
#include <stdlib.h>
#include <string>

namespace Json {

   /////////////////////////////////////////////////////////////////////////////////////////////////

   /// The type of a JSON value.
   enum Type {
      JNULL,
      JBOOL,
      JSTRING,
      JNUMBER,
      JOBJECT,
      JARRAY
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////

   /// A JSON value.
   struct Value {
      /// The value's name if it is part of an object, "" otherwise. Never null.
      const char *name_;

      /// The value's textual representation for simple values. If this is an object
      /// or array, pointer to the first element.
      const char *value_;

      /// The next element for object and array members, or null
      Value *next_;

      /// Returns the type of this value.
      Type type() const
      {
         return (Type) name_[-1];
      }

      /// Interprets the value as an integer.
      /// Throws if this is not a number or boolean.
      int asInt() const;

      /// Interprets the value as a floating point value.
      double asDouble() const
      {
         return atof(value_);
      }

      /// Returns the value's string representation.
      /// Throws if this is an array or object.
      const char *asString() const;

      /// Returns the boolean interpretation of the value.
      /// false, null and zero numbers (integer or float) are falsy.
      /// All other values are thruthy.
      bool asBool() const;

      /// Returns the number of array of object members.
      /// Returns 0 for simple types.
      size_t length() const;

      /// Returns the first child or NULL if the array/object is empty.
      /// Throws if this is not an array or object.
      const Value* children() const;

      /// Array element access.
      const Value& get(int i) const;
      const Value& operator[](int i) const { return get(i); }

      /// Object member access.
      const Value& get(const char *name) const;
      const Value& get(const std::string &key) const { return get(key.c_str()); };
      const Value& operator[](const char *key) const { return get(key); }
      const Value& operator[](const std::string &key) const { return get(key.c_str()); }

   };

   /////////////////////////////////////////////////////////////////////////////////////////////////

   enum ParseMode {
      NON_DESTRUCTIVE,  // copy the source before parsig
      DESTRUCTIVE       // don't copy, ovewrite source buffer
   };

   class Tree
   {
   private:
      struct Chunk {
         char *eofs_;   // end of free space
         Chunk *next_;  // next chunk
      };

      Chunk *head_;
      Value* root_;
      Value *parseInternal(char *source, const char **error_pos, const char **error_desc);
      void parseInternal(char *source);

      Tree(const Tree&);                // not implemented
      void operator=(const Tree&);      // not implemented
   public:
      Tree();
      Tree(char *source, ParseMode mode = NON_DESTRUCTIVE);
      Tree(const char *source);
      Tree(const std::string &source);
      ~Tree();

      char *malloc(size_t size);

      void parse(char *source, ParseMode mode = NON_DESTRUCTIVE);
      void parse(const char *source);
      void parse(const std::string &source);

      const Value& root() const;

      /// Convenience methods for transparent root access.
      size_t length() const { return root_->length(); }
      Type type() const { return root_->type(); }
      const Value& get(int i) const { return root_->get(i); }
      const Value& get(const char *name) const { return root_->get(name); }
      int asInt() const { return root_->asInt(); }
      double asDouble() const { return root_->asDouble(); }
      const char *asString() const { return root_->asString(); }
      bool asBool() const { return root_->asBool(); }
      const Value& operator[](int i) const { return root_->get(i); }
      const Value& operator[](const char *key) const { return root_->get(key); }
      const Value& operator[](const std::string &key) const { return root_->get(key); }
   };

   /////////////////////////////////////////////////////////////////////////////////////////////////

   class SyntaxError: public std::runtime_error
   {
   public:
      // The source offset where the error was detected.
      const size_t offset_;

   public:
      ~SyntaxError() throw();
      SyntaxError(size_t offset, const char *message);
   };

}

#endif

