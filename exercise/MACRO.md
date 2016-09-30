# Predefined Macros of OS

The [Predefined Macros site](http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system) has a very complete list of checks. Here are a few of them:

## Windows

Taken from the Visual C docs, the most common ones are:

_WIN32   Both 32 bit and 64 bit
_WIN64   64 bit only

## Unix (Linux, *BSD, Mac OS X)

See this related question on some of the pitfalls of using this check.

### unix
__unix
__unix__

### Mac OS X

__APPLE__
__MACH__

Both are defined; checking for either should work.

### Linux

http://www.faqs.org/docs/Linux-HOWTO/GCC-HOWTO.html

__linux__

### FreeBSD

http://www.freebsd.org/doc/en/books/porters-handbook/porting-versions.html

__FreeBSD__


