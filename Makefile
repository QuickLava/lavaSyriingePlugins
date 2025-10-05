#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

export TOOLS 	:= $(CURDIR)/tools
export LIB 		:= $(CURDIR)/lib


.PHONY: all lavaInjectLoader lavaPSAExtensions lavaGameTweaks lavaNeutralSpawns lavaFrameHeapWatch

all: lavaInjectLoader lavaPSAExtensions lavaGameTweaks lavaNeutralSpawns lavaFrameHeapWatch

lavaInjectLoader:
	$(MAKE) -C lavaInjectLoader
	@cp lavaInjectLoader/$@.rel ./output/$@.rel
	
lavaPSAExtensions:
	$(MAKE) -C lavaPSAExtensions
	@cp lavaPSAExtensions/$@.rel ./output/$@.rel
	
lavaGameTweaks:
	$(MAKE) -C lavaGameTweaks
	@cp lavaGameTweaks/$@.rel ./output/$@.rel
	
lavaNeutralSpawns:
	$(MAKE) -C lavaNeutralSpawns
	@cp lavaNeutralSpawns/$@.rel ./output/$@.rel
	
lavaFrameHeapWatch:
	$(MAKE) -C lavaFrameHeapWatch
	@cp lavaFrameHeapWatch/$@.rel ./output/$@.rel
	
clean:
	@rm ./output/*.rel
	$(MAKE) -s -C lavaInjectLoader clean
	$(MAKE) -s -C lavaPSAExtensions clean
	$(MAKE) -s -C lavaGameTweaks clean
	$(MAKE) -s -C lavaNeutralSpawns clean
	$(MAKE) -s -C lavaFrameHeapWatch clean
