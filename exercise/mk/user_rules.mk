# User define routines
# Retrieves a host part of the given string (without port).
# Param:
#   1. String to parse in form 'host[:port]'.
hSample_host = $(firstword $(subst :, ,$1))

# Returns a port (if any).
# If there is no port part in the string, returns the second argument
# (if specified).
# Param:
#   1. String to parse in form 'host[:port]'.
#   2. (optional) Fallback value.
hSample_port = $(or $(word 2,$(subst :, ,$1)),$(value 2))

#Usage:
#
#	$(call hSample_host,foo.example.com) # foo.example.com
#	$(call hSample_port,foo.example.com,80) # 80
#	
#	$(call hSample_host,ssl.example.com:443) # ssl.example.com
#	$(call hSample_port,ssl.example.com:443,80) # 443

# Helper retrieves a current last dir name
hDirName = $(lastword $(subst /, ,$(d)))
hDir1Name = $(lastword $(subst /, ,$(abspath $(d)/..)))


ifeq ("$(BUILD_VERBOSE)","1")
Q :=
vecho = @echo
else
Q := @
vecho = @true
endif

#%.o: %.c
#    $(vecho) "-> Compiling $@"
#    $(Q)$(CC) $(CFLAGS) -c $< -o $@


define testcase
	@echo "Note: VERBOSE=$(VERBOSE), if need more verbose please set 'y'"
	@if [ "$(VERBOSE)" = "y" ]; then \
		$(2) && echo "TestCase '$(1)' succ {{{2}}}" || echo "TestCase '$(1)' fail {{{2}}}"; \
	else \
		$(2) > /dev/null 2>&1 && echo "TestCase '$(1)' succ {{{2}}}" || echo "TestCase '$(1)' fail {{{2}}}"; \
	fi
endef
