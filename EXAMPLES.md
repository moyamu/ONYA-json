ONYA-json Example code and programs
=================================================================

JSON Pretty Printer
-----------------------------------------------------------------

If all you want to do is formatting JSON documents there is no
need for a complete parser. You can use **Json::prettyPrint()**,
which is basically a simplified version of the parser that
produces neatly formatted output.

An example is shown in the **jsonpp.c** program. You can use
it as a command line filter, e.g.

    wget -O - http://host/path | jsonpp

