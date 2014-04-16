ONYA-json
=================================================================

A simple and fast JSON parser for C++

What it does:

* parse UTF-8 encoded JSON source
* navigate JSON tree with get() and/or operator[] syntax
* convert JSON to C++ types, e.g. boolean → string
* basic validation - detects most common syntax errors
* optional: destructive parsing for better performance

What it doesn't:

* support source encodings other than UTF-8
* handle comments
* manipulate or generate JSON
* provide efficient random access − list and object access
  cost is O(length) 

How it works:

* Two classes: **Tree** (= JSON document) and **Value** (=JSON node)
* Parsing is string based, chopping the source into pieces 
  and storing pointers to the pieces.
* Memory is managed by the Tree. All Values are invalidated 
  when the Tree is destroyed. 
  With destructive parsing, the user must not alter the source buffer
  during the Tree's lifetime.
* JSON to C++ type conversion is always explicit: 
  asBool(), asDouble(), ...
  
Build
-----------------------------------------------------------------

Everything is in **json.cc** / **json.h** - just compile it with 
your favourite C++ compiler and settings.

To build and run the tests under Linux/GCC:

    g++ -o tests tests.cc _test.cc json.cc -lrt
    ./tests

Use
-----------------------------------------------------------------

    using namespace Json;

    const char *source = ... // e.g., read from file
    Tree tree(source);

    // Simple types
    std::string name = tree.get("name"); 
    int age = tree.get("age").asInt();
    double weight = tree.get("weight").asDouble();

    // Arrays and objects
    int numberOfChildren = tree.get("children").size();
    Value &secondChild = tree.get("children").get(2);
    for (int i = 0; i < children.size(); ++i) {
       const char *name = children.get(i).get("name").asString();
    }

    // Iterating
    for (const Value *i = &parent.children(); i; i = i->next_) {
	// ...
    }

    // operator syntax
    int year = tree["children"][1]["year"].asInt();
        
    tree.clear();	// Frees all memory


Destructive vs. non-destructive parsing:

    const char *constSource;
    char *mutableSource;

    // (1) non-destructive, copies source
    Tree a(constSource);
    Tree b; b.parse(constSource);
    Tree c; c.parse(mutableSource);	// defaults to NON_DESTRUCTIVE

    // (2) destructive, buffer managed by user
    Tree d(mutableSource,DESTRUCTIVE);
    Tree e; e.parse(mutableSource,DESTRUCTIVE);

    // (3) destructive, buffer managed by Tree
    Tree f;
    char *buffer = f.malloc(fileSize + 1);
    read(file,buffer,fileSize);
    buffer[fileSize] = 0;
    f.parse(buffer, DESTRUCTIVE);

