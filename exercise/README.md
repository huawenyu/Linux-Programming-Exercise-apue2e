<!-- Enable two-step security -->

# Makefile

https://github.com/aostruszka/nonrec-make

## quickstart

There have more detail about [how to make](MAKE.md).

```
$ make all && make -s VERBOSE=n test

$ make all
$ make test
$ valgrind --tool=memcheck --track-origins=yes --leak-check=yes --verbose ./test
$ make dist_clean

### if don't want to make test
$ make
```

## How to use the nonrec-make in new proj

1. copy dir `mk`
2. ln -s .mk/Makefile Makefile
3. create `Rules.mk` like:

    TOPBIN := $(TOP)/bin
    TOPLIB := $(TOP)/binlib
    TOPTEST := $(TOP)/bintest
    _forcedir := $(shell mkdir -p $(TOPBIN))
    _forcedir := $(shell mkdir -p $(TOPLIB))
    _forcedir := $(shell mkdir -p $(TOPTEST))

    LIBDIRS = $(TOPLIB)
    INCLUDES = $(d)/src/include

    CFLAGS := -g -O0 -std=gnu99 -DDEBUG -Wall -Werror -Wextra \
    	  -Wno-unused-parameter \
    	  -Wno-sign-compare \
    	  -fms-extensions
    ARFLAGS = rc
    LDLIBS += -lm
    LDFLAGS += -Wl,-R$(TOPLIB)

    SUBDIRS = src test

    # Please care about the double colon for not override others 'test'
    test::
    	@echo "test top {{{2}}}"

    CLEAN_$(d) = $(d)/cscope.* $(d)/tag* $(d)/obj \
    	     $(TOPBIN) $(TOPLIB) $(TOPTEST)

4. ln -s ./Rules.mk Rules.top
5. make
6. --The End--

## How to creare new sub-proj
```
# if create new test1 dir
cd test1
ln -s ../Makefile Makefile
# edit your currentfile
vi main.mk
```

# TODOS

- [x] Makefile
