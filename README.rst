=============================================
cmdline: A simple command line parser for C++
=============================================

About
-----

This is a simple command line parser for C++.

- Easy to use
- Only one header file
- Automatic type check

Sample
------

Here show sample usages of cmdline.

Normal usage
============

This is an example of simple usage.

::

  // include cmdline.h
  #include "cmdline.h"
  
  int main(int argc, char *argv[])
  {
    // create a parser
    cmdline::parser a;
    
    // add specified type of variable.
    // 1st argument is long name
    // 2nd argument is short name (no short name if '\0' specified)
    // 3rd argument is description
    // 4th argument is mandatory (optional. default is false)
    // 5th argument is default value  (optional. it used when mandatory is false)
    a.add<string>("host", 'h', "host name", true, "");
    
    // 6th argument is extra constraint.
    // Here, port number must be 1 to 65535 by cmdline::range().
    a.add<int>("port", 'p', "port number", false, 80, cmdline::range(1, 65535));
    
    // cmdline::oneof() can restrict options.
    a.add<string>("type", 't', "protocol type", false, "http", cmdline::oneof<string>("http", "https", "ssh", "ftp"));
    
    // Boolean flags also can be defined.
    // Call add method without a type parameter.
    a.add("gzip", '\0', "gzip when transfer");
    
    // Run parser.
    // It returns only if command line arguments are valid.
    // If arguments are invalid, a parser output error msgs then exit program.
    // If help flag ('--help' or '-?') is specified, a parser output usage message then exit program.
    a.parse_check(argc, argv);
    
    // use flag values
    cout << a.get<string>("type") << "://"
         << a.get<string>("host") << ":"
         << a.get<int>("port") << endl;
    
    // boolean flags are referred by calling exist() method.
    if (a.exist("gzip")) cout << "gzip" << endl;
  }

Here are some execution results:

::

  $ ./test
  usage: ./test --host=string [options] ... 
  options:
    -h, --host    host name (string)
    -p, --port    port number (int [=80])
    -t, --type    protocol type (string [=http])
        --gzip    gzip when transfer
    -?, --help    print this message

::

  $ ./test -?
  usage: ./test --host=string [options] ... 
  options:
    -h, --host    host name (string)
    -p, --port    port number (int [=80])
    -t, --type    protocol type (string [=http])
       --gzip    gzip when transfer
    -?, --help    print this message

::

  $ ./test --host=github.com
  http://github.com:80

::

  $ ./test --host=github.com -t ftp
  ftp://github.com:80

::

  $ ./test --host=github.com -t ttp
  option value is invalid: --type=ttp
  usage: ./test --host=string [options] ... 
  options:
    -h, --host    host name (string)
    -p, --port    port number (int [=80])
    -t, --type    protocol type (string [=http])
        --gzip    gzip when transfer
    -?, --help    print this message

::

  $ ./test --host=github.com -p 4545
  http://github.com:4545

::

  $ ./test --host=github.com -p 100000
  option value is invalid: --port=100000
  usage: ./test --host=string [options] ... 
  options:
    -h, --host    host name (string)
    -p, --port    port number (int [=80])
    -t, --type    protocol type (string [=http])
        --gzip    gzip when transfer
    -?, --help    print this message

::

  $ ./test --host=github.com --gzip
  http://github.com:80
  gzip

Extra Options
=============

- rest of arguments

Rest of arguments are referenced by rest() method.
It returns vector of string.
Usualy, they are used to specify filenames, and so on.

::

  for (int i = 0; i < a.rest().size(); i++)
    cout << a.rest()[i] << endl\;

- footer

footer() method is add a footer text of usage.

::

  ...
  a.footer("filename ...");
  ...

Result is:

::

  $ ./test
  usage: ./test --host=string [options] ... filename ...
  options:
    -h, --host    host name (string)
    -p, --port    port number (int [=80])
    -t, --type    protocol type (string [=http])
        --gzip    gzip when transfer
    -?, --help    print this message

- program name

A parser shows program name to usage message.
Default program name is determin by argv[0].
set_program_name() method can set any string to program name.

Process flags manually
----------------------

parse_check() method parses command line arguments and
check error and help flag.

You can do this process mannually.
bool parse() method parses command line arguments then
returns if they are valid.
You should check the result, and do what you want yourself.

(For more information, you may read test2.cpp.)
