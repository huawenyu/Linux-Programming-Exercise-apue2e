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

SUBDIRS = process

# Please care about the double colon for not override others 'test'
test::
	@echo "test top {{{2}}}"

CLEAN_$(d) = $(d)/cscope.* $(d)/tag* $(d)/obj \
	     $(TOPBIN) $(TOPLIB) $(TOPTEST)
