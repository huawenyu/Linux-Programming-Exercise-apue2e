Process_child := $(TOPBIN)/$(hDirName)_child

TARGETS := $(Process_child)

SRCS := child_alarm.c
$(Process_child)_DEPS = $(OBJS_$(d))

test::
	@echo "Fork process child with alarm {{{2}}}"
	$(Process_child)

