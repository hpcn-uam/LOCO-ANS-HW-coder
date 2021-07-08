
SUBTARGET := all

ALL_TARGETS := ANS_coder DMA_engines File_writer input_buffers output_stack subsym_gen 


BASIC_TARGETS := DMA_engines TSG_coder

.PHONY:	all all_modules $(ALL_TARGETS)

all: $(BASIC_TARGETS)

all_modules: SUBTARGET := all_modules
all_modules: $(ALL_TARGETS)

$(ALL_TARGETS):
	make -C $@ $(SUBTARGET)

