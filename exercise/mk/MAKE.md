# CONTENTS

  Makefile targets
  Common build options (tests, optimization level)
  Build system overview
  Adding new directories
    Common per-Rules.mk variable names
    Common variables to reference
    Specifying include directories
  General notes on the makefile framework
    Terminology


# Makefile targets

Building execlude the test module:

    $ make
    $ make dist_clean

Building include the test module:

    $ make TARGET=test
    $ make dist_clean

Call every 'test' which define in every Rules.mk:

    $ make test

    ```make
    TEST_RUN_2 := $(d)/test_$(hDirName)

    # This not-override phony belong to [Double-Colon Rules](https://www.gnu.org/software/make/manual/html_node/Double_002dColon.html#Double%5F002dColon)
    test::
    	echo "run from allocator"
    	$(TEST_RUN_2)
    ```

Troubleshooting using remake or dump:

    $ remake -x
    $ remake -x all
    $ make all print-TARGETS    <<< print out the var 'TARGETS'
    $ make DUMP=true all        <<< print all shell command

Makefile and final.mk are the best places to look to see what
targets are defined. Some of the more useful top-level targets
(defined in Makefile and final.mk):
* test, Runs the test suite (including unit tests).
* all, Equivalent of "make kernel_only image.out".
* clean_all,
  Remove all build artifacts from the standard build. (Not
  user space components built with TARGET=unix or TARGET=test.)

* dir, tree, all,
  The default target is 'dir', which builds the current
  directory's targets. 'tree' includes all subtree targets and
  'all' builds all targets.

* clean, clean_tree, clean_all,
  Clean the current directory's build artifacts for the
  destination target specified by TARGET (defaults to 'fg'). For
  example, if you have built for TARGET=unix, running "make
  clean" will not remove any build artifacts for it -- only
  those for TARGET=am.

  Use clean_tree to include subdirectories, clean_all to clean
  all build artifacts for TARGET.

* tests, tree_tests, all_tests.
  For 'tests', if the current directory has targets in the
  special TESTS or GTESTS variables, build them and run them.
  The 'tree' prefix includes all subdirectories; the 'all'
  prefix includes all tests.

# Common build options

## make CONFIG_OPT=no
Turn off compiler optimizationito get cleaner backtraces.

##  make TARGET=unix
Building a test harness or standalone executable.  
The "TARGET" variable is used to indicate whether you wish to
build for, your PC, or an automated test harness to run on your PC.

    For example, to build a standalone executable (if the memory
    you're working with supports it), run something similar to this
    example for memory:

      $ cd src/memory
      $ make TARGET=test
      $ ./test_memory

    Valid target options are:
      am    The default, release and optimize.
      unix  Stand-alone executable to run on local.
      test  Stand-alone executable containing several automated test
            routines or unit tests.

# Build system overview

## Features

### Support make from any where

### Support mult-targets on different platform-OS

If the default target is 'am', so we can use make TARGET=unix to switch platform:

    ```make
    TARGETS := $(LIB_MEM_ALLOCATOR)

    SRCS_am :=
    SRCS_unix :=
    SRCS := $(notdir $(wildcard $(d)/*.c))

    $(LIB_MEM_ALLOCATOR)_DEPS = $(OBJS_$(d))
    INCLUDES_$(d) := $(d)

    # make TARGET=unix
    ifeq ($(TARGET),unix)
           SRCS_VPATH = test
           SRCS := $(SRCS) $(SRCS_unix)
           $(LIB_MEM_ALLOCATOR)_DEPS = $(OBJS_$(d))
    endif
    ```

### Support multi-targets from different MACROs

Current this make framework can't support mult-target with different CFLAGS,
so here we create mult-sub-dirs which stand for different CFLAGS,
but all subdirs Rules.mk share the same source code from parent dir.

    ```make
    allocator
    ├── fix
    │   ├── Makefile -> ../Makefile
    │   └── Rules.mk
    ├── Makefile -> ../Makefile
    ├── Rules.mk		<--- only list subdirs
    ├── std
    │   ├── Makefile -> ../Makefile
    │   └── Rules.mk
    ├── test_main.c		<--- the source code shared by all sub-dirs
    ├── test_malloc.c	<--- should define use-std-malloc or use-adv-mem

    $ cat std/Rules.mk

    TEST_MEM_ALLOCATOR_std := $(TOPTEST)/test_$(hDir1Name)_std

    TARGETS := $(TEST_MEM_ALLOCATOR_std)

    CFLAGS += -DUSE_MALLOC
    # vpath must be relative dir, not use $(d)
    SRCS_VPATH := ..

    # if use wildcard, please use $(d), otherwise list file name directly
    SRCS := test_main.c test_malloc.c
    #SRCS := $(notdir $(wildcard $(d)/*.c))

    $(TEST_MEM_ALLOCATOR_std)_DEPS = $(OBJS_$(d))
    $(TEST_MEM_ALLOCATOR_std)_LIBS := -l$(LIB_MEM_ALLOCATOR_lib)
    LIBDIRS_$(d) := $(TOPLIB)
    INCLUDES_$(d) := $(LIB_MEM_ALLOCATOR_hdr)
    ```

#### Even support one file have different MACROs

    ```make
    $(OBJPATH)/dir_1b_file2.o : CFLAGS += -ansi
    ```

### Support unit test build & run
> $ make TARGET=test
> $ make test

    ```make
    LIB_MEM_ALLOCATOR := $(TOPLIB)/lib$(hDirName).a
    LIB_MEM_ALLOCATOR_test := $(d)/test_$(hDirName)

    TARGETS := $(LIB_MEM_ALLOCATOR)

    SRCS_test := test_main.c
    SRCS := $(notdir $(wildcard $(d)/*.c))

    $(LIB_MEM_ALLOCATOR)_DEPS = $(OBJS_$(d))

    # make TARGET=test
    ifeq ($(TARGET),test)
           TARGETS += $(LIB_MEM_ALLOCATOR_test)
           SRCS := $(SRCS) $(SRCS_test)
           $(LIB_MEM_ALLOCATOR_test)_DEPS = $(OBJS_$(d))
    endif
    ```

### Support sources from subdirs or parent-dirs

If test_main.c in the sub-dir 'test':

    ```make
    LIB_MEM_ALLOCATOR_test := $(d)/test_$(hDirName)

    TARGETS := $(LIB_MEM_ALLOCATOR_test)

    # The SRC cann't have path like test/test_main.c,
    # So use SRCS_VPATH to solve the mulitple sub-dirs issue.
    SRCS_test := test_main.c
    SRCS := $(notdir $(wildcard $(d)/*.c))

    # And the vpath use relative-path, don't use $(d)
    SRCS_VPATH = test
    TARGETS += $(LIB_MEM_ALLOCATOR_test)
    SRCS := $(SRCS) $(SRCS_test)
    $(LIB_MEM_ALLOCATOR_test)_DEPS = $(OBJS_$(d))
    ```

### Support special clean

    $ make dist_clean

    ```make
    # The clean need abs-path, please use $(d)
    CLEAN_$(d) := $(d)/cscope.* $(d)/tag* $(d)/obj $(LIB_MEM_ALLOCATOR_test)
    ```

### Note: if library, should have global reference var

    ```make
    LIB_MEM_ALLOCATOR := $(TOPLIB)/lib$(hDirName).a
    LIB_MEM_ALLOCATOR_lib := $(hDirName)
    LIB_MEM_ALLOCATOR_test := $(d)/test_$(hDirName)
    LIB_MEM_ALLOCATOR_dir := $(d)
    LIB_MEM_ALLOCATOR_hdr := $(d)
    ```

## File & Dir

  mk/*
    The non-recursive make framework used to build user space.

  Rules.mk
    The top-level user space makefile, included by mk/Makefile.
    Contains significant real top-level targets.

  final.mk
    Included at the end of mk/Makefile. Primarily defines phony
    targets to make it easier to build the desired real targets.

## Adding a new library (Rules.mk)

1. Link Makefile
   ```shell
   $ ln ../Makefile Makefile
   ```
2. The most basic Rules.mk for a library included in init:
   ```make
   SRCS := *.c
   LIB_SOMETHING := $(OBJPATH)/libsomething.o
   $(LIB_SOMETHING)_DEPS = $(OBJS_$(d))
   TARGETS = $(LIB_SOMETHING)
   ```

  For the above, init_libs.mk would need a line to include
  "$$(LIB_SOMETHING)" in the list of dependencies for init. The reason
  for the double-$ is to defer evaluating $(LIB_SOMETHING) in case its
  Rules.mk has not been included by the time init_libs.mk is first
  seen.

  The reason we explicitly specify $(OBJPATH) for libsomething.o is
  that when the Rules.mk file is evaluated, any target with no path is
  automatically placed in $(OBJPATH), but when init is linked, it will
  also need to know the full path to libsomething.o -- and in the
  context of its Rules.mk file, $(OBJPATH) has a different value. This
  is also why we use immediate assignment (":=") instead of deferred
  variable assignment ("=").

3. Sample Rules have two different target: am, test

   ```make
   # Every lib should have uniq global name for reference
   LIB_MEM_ALLOCATOR := $(TOPLIB)/lib$(hDirName).a
   LIB_MEM_ALLOCATOR_lib := $(hDirName)
   LIB_MEM_ALLOCATOR_test := $(d)/test_$(hDirName)
   LIB_MEM_ALLOCATOR_dir := $(d)
   LIB_MEM_ALLOCATOR_hdr := $(d)

   TARGETS := $(LIB_MEM_ALLOCATOR)

   # Here must use "$(d)/*.c" to get the full path
   SRCS_am :=
   SRCS_unix :=
   # The SRC cann't have path like test/test_main.c,
   # So use SRCS_VPATH to solve the mulitple sub-dirs issue.
   SRCS_test := test_main.c
   SRCS := $(notdir $(wildcard $(d)/*.c))

   $(LIB_MEM_ALLOCATOR)_DEPS = $(OBJS_$(d))
   INCLUDES_$(d) := $(d)

   # make TARGET=test
   ifeq ($(TARGET),test)
          SRCS_VPATH += test
          TARGETS += $(LIB_MEM_ALLOCATOR_test)
          SRCS := $(SRCS) $(SRCS_test)
          $(LIB_MEM_ALLOCATOR_test)_DEPS = $(OBJS_$(d))
   endif

   CLEAN_$(d) := $(d)/cscope.* $(d)/tag*

   # make test
   test::
   	@echo "test memory allocator {{{2}}}"
   	$(LIB_MEM_ALLOCATOR_test)
   ```

For the above, init_libs.mk would need a line to include
"$$(LIB_SOMETHING)" in the list of dependencies. The reason
for the double-$ is to defer evaluating $(LIB_SOMETHING) in case its
Rules.mk has not been included by the time init_libs.mk is first seen.

The reason we explicitly specify $(OBJPATH) for libsomething.o is
that when the Rules.mk file is evaluated, any target with no path is
automatically placed in $(OBJPATH), but when init is linked, it will
also need to know the full path to libsomething.o -- and in the
context of its Rules.mk file, $(OBJPATH) has a different value. This
is also why we use immediate assignment (":=") instead of deferred
variable assignment ("=").

## Common variable per-Rules.mk

Several variables are cleared before including any Rules.mk file.
The full list can be found in mk/headers.mk.

The following are frequently used makefile variables for the
user space non-recursive make framework. All are within the scope
of the current Rules.mk file unless otherwise indicated.

* SUBDIRS
  Subdirectories to descend into to include Rules.mk files.

* INCLUDES_$(d)
  Search path for header files. See "specifying include
  directories", below for an explanation. (See mk/config.mk for
  the default INCLUDES.)

* CPPFLAGS_$(d)
  C preprocessor flags to define for C and C++ files.

* CFLAGS_$(d)
  Compiler options to use for C files.

* CXXFLAGS_$(d)
  Compiler options to use for C++ files.

* SRCS, SRCS_EXCLUDES
  Source files to compile (SRCS) or exclude from compiling
  (SRCS_EXCLUDES). Can use wildcards directly. Combined to form
  $(OBJS_$(d)) for use as prerequisites for target libraries or
  executables.

  To make it easier to include all but a few source files based on
  platform config, a "selective_srcs_excludes" definition exists.

  An abbreviated example of Rules.mk:
  ```make
  SRCS := *.c
  SRCS-$(HAVE_FEATURE1) += \
          feature1_code1.c \
          feature1_code2.c
  SRCS-$(HAVE_FEATURE2) += \
          feature2_code.c
  SRCS_EXCLUDES := $(call selective_srcs_excludes)
  ```
  All C files in the current directory are included, but if
  HAVE_EXPLICIT_PROXY or HAVE_WANOPT are not set, the files they
  list will be excluded. Note that webcache_urlmatch.c is in both
  SRCS options for HAVE_EXPLICIT_PROXY and HAVE_WANOPT. It will be
  compiled if either or both are set to 'y' in the platform
  config.

* SRCS_VPATH
  Specify a search path outside of the current directory for
  source files. May be a relative path.

* BUILD_SRCS, BUILD_SRCS_EXCLUDES, BUILD_SRCS_VPATH
  As with the above three, but for BUILD_TARGET targets, which are
  built to run on the build machine, either as a part of the build
  process or as a tool for later use.

* LIB_xxxx := $(OBJPATH)/libxxxx.o
  Library to be built with source in current directory.

* LIB_xxxx_SO := $(TOPLIB)/libxxxx.so
  Shared library to be built with source in top library directory.

* <target>_DEPS = $(OBJS_$(d))
  Prerequisites for <target> -- for example, $(LIB_xxxx)_DEPS.
  Note: Do not use immediate assignment (":=") when referencing
  $(OBJS_$(d)), as it is undefined until after post-processing on
  the Rules.mk file has been done.

  When adding an external object file or library as a
  prerequisite, reference it using an escaped $ to delay variable
  expansion:

  DAEMON_LIBS += $$(STRLCPY_O)

* TARGETS := $(LIB_xxxx)
  Targets to build for current directory.

  If the current directory simply contains additional source files
  to be included in the parent directory's target, instead of
  wrapping everything everything into a single library or object
  file, simply include $(SUBDIRS_TGTS) in the parent directory's
  target's _DEPS. See the Rules.mk files in antispam and its
  common, engine and filters subdirectories for an example.

* BUILD_TARGETS
  Targets to build for current directory that will run on the
  build machine.

* TESTS
  A special version of TARGETS, whose contents will be included in
  TARGETS when TARGET=test (implicit if "make tests" is
  run"). These will be automatically executed. (Use "target_TEST =
  command" to specify an alternate command to run.)

* GTESTS
  A version of TESTS that links in the Google Test framework's
  shared library. These tests are run with the "shuffle" option to
  randomize the order they are run in.

* CLEAN
  Extra files that should be cleaned up when "make clean" is run
  that are not covered in the TARGETS or OBJS lists.

* CLEAN_EXCLUDES
  Files that should _not_ be cleaned up when "make clean" is run.
  This should almost never be used. It exists to allow specifying
  a file in TESTS or TARGETS or similar that is checked into
  revision control.

## Common variables to reference

* $(d)          - Current directory of current Rules.mk file.
* $(..)         - Parent directory of current Rules.mk file. Can also use
                  $(parent_dir)
* $(OBJS_$(d))  - The list of object files created from SRCS and
                  SRCS_EXCLUDES.
* $(OBJPATH)    - Build output directory for current Rules.mk file

* $(BUILD_OBJS_$(d)) - As with OBJS, but for BUILD_SRCS.
* $(BUILD_OBJPATH) - Build output directory for BUILD_TARGETS in
                     current Rules.mk file

* $(TOP)        - Top directory of source tree.
* $(TOPLIB)     - Top library directory of source tree. (Avoid using this.)

* $(call get_subtree,OBJS,$(d))
  Can be used to specify prerequisites for a target. Useful if the
  source files are in subdirectories, to avoid linking into
  intermediate object files or using SRCS_VPATH (see antispam as
  an example).


## Specifying include directories

Several include directories are used across all of by default.
See the INCLUDES definition in config.mk.

* INCLUDES_$(d) := $(INCLUDES_$(..))
    Same include directories as parent(s):

* INCLUDES_$(d) := $(d)/include
    Subdirectory of the current directory:

* Top-level include directory:
  INCLUDES_$(d) := $(TOP)/scanunit/framework
  INCLUDES_$(d) := $(JEMALLOC_DIR)/include

  ** See mk/config-paths.mk for other variables referring to
  directories in the code base.

## Terminology

* BUILD
  The build machine on which tools are being run (or compiled to run).

* TARGET
  The system for which the tools generate code. (Normally the release.)

* TARGETS
  The things we want to build. Executables, object files, libraries.

  Yes, it is confusing that TARGET and TARGETS have such different
  meanings.

## Troubleshooting

Use echo to print the VALUE point-to var's value.
```Shell
  $ make VALUE=TARGETS echo tests

  Reading build rules...
  Build rules read.
  TARGETS = <home>/project/algorithm/lib/liballocator.a
  <home>/project/algorithm/src/memory/allocator/test_allocator
  make: Nothing to be done for `tests'.

  ### or we can use VALUE point-to that value name in out Rules.mk
       VALUE = $(TESTS)_DEPS
  $ make echo tests
```

# General GNU-make

## Variable assignment

https://www.gnu.org/software/make/manual/html_node/Variables-in-Recipes.html

* VARIABLE = value      Lazy Set
  Normal setting of a variable - values within it are recursively expanded when the variable is used, not when it's
  declared
  ```make
  HELLO = world
  HELLO_WORLD = $(HELLO) world!
  # This echoes "world world!"
  echo $(HELLO_WORLD)

  HELLO = hello
  # This echoes "hello world!"
  echo $(HELLO_WORLD)
  ```
* VARIABLE := value     Immediate Set
  Setting of a variable with simple expansion of the values inside - values within it are expanded at declaration time.

  ```make
  HELLO = world
  HELLO_WORLD := $(HELLO) world!
  # This echoes "world world!"
  echo $(HELLO_WORLD)

  HELLO = hello
  # Still echoes "world world!"
  echo $(HELLO_WORLD)

  HELLO_WORLD := $(HELLO) world!
  # This echoes "hello world!"
  echo $(HELLO_WORLD)
  ```

* VARIABLE ?= value       Set If Absent
  Setting of a variable only if it doesn't have a value.

* VARIABLE += value       Append
  Appending the supplied value to the existing value (or setting to that value if the variable didn't exist)

