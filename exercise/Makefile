$(info Reading build rules...)

SHELL := /bin/bash
RUNDIR := $(CURDIR)
ifndef TOP
TOP := $(shell \
       top=$(RUNDIR); \
       while [ ! -r "$$top/Rules.top" ] && [ "$$top" != "" ]; do \
           top=$${top%/*}; \
       done; \
       echo $$top)
endif

MK := $(TOP)/mk

.PHONY: dir tree all clean clean_all clean_tree dist_clean
.PHONY: test

test::
	@echo "Runing test ... {{{1}}}"

# Default target when nothing is given on the command line.  Reasonable
# options are:
# "dir"  - updates only targets from current directory and its dependencies
# "tree" - updates targets (and their dependencies) in whole subtree
#          starting at current directory
# "all"  - updates all targets in the project
.DEFAULT_GOAL := dir

dir : dir_$(RUNDIR)
tree : tree_$(RUNDIR)

clean : clean_$(RUNDIR)
clean_tree : clean_tree_$(RUNDIR)

# [Debugging Makefiles](http://www.drdobbs.com/tools/debugging-makefiles/197003338)
# To help with debugging makefiles.
# Example:
#   $ make test print-TARGETS
#print-%: ; @$(error $* is $($*))
#           $(filter-out print-%, $(MAKECMDGOALS)): $(filter print-%, $(MAKECMDGOALS))
#
#print-%:
#	@echo '$*=$($*)'
#	@echo '  origin = $(origin $*)'
#	@echo '  flavor = $(flavor $*)'
#	@echo '   value = $(value  $*)'
#
print-%: ; @$(error $*=$($*) ($(value $*)) (from $(origin $*)))

# Example:
#   $ make DUMP=true test
#
#OLD_SHELL := $(SHELL)
#SHELL = $(warning [$@ ($^) ($?)])$(OLD_SHELL)
#
ifdef DUMP
OLD_SHELL := $(SHELL)
SHELL = $(warning [$@])$(OLD_SHELL) -x
endif

# $(d) keeps the path of "current" directory during tree traversal and
# $(dir_stack) is used for backtracking during traversal
d := $(TOP)
dir_stack :=

include $(MK)/header.mk
include $(MK)/footer.mk

# Automatic inclusion of the skel.mk at the top level - that way
# Rules.top has exactly the same structure as other Rules.mk
include $(MK)/skel.mk

.SECONDEXPANSION:
$(eval $(value HEADER))
include $(TOP)/Rules.top
$(eval $(value FOOTER))

# Optional final makefile where you can specify additional targets
-include $(TOP)/final.mk

# This is just a convenience - to let you know when make has stopped
# interpreting make files and started their execution.
#$(info Rules generated $(if $(BUILD_MODE),for "$(BUILD_MODE)" mode,)...)
$(info Build rules read.)
