#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

export TOOLS 	:= $(CURDIR)/tools
export LIB 		:= $(CURDIR)/lib


.PHONY: all lavaInjectLoader lavaPSAExtensions

all: lavaInjectLoader lavaPSAExtensions

lavaInjectLoader:
	$(MAKE) -C lavaInjectLoader
	@cp lavaInjectLoader/$@.rel ./output/$@.rel
	
lavaPSAExtensions:
	$(MAKE) -C lavaPSAExtensions
	@cp lavaPSAExtensions/$@.rel ./output/$@.rel
	
clean:
	@rm ./*.rel
	$(MAKE) -s -C lavaInjectLoader clean
	$(MAKE) -s -C lavaPSAExtensions clean
