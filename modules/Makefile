
SUBTARGET := all

BASIC_TARGETS := DMA_engines TSG_coder File_writer
ALL_TARGETS := $(BASIC_TARGETS) ANS_coder input_buffers output_stack subsym_gen 


.PHONY:	all all_modules $(ALL_TARGETS)

all: $(BASIC_TARGETS)

all_modules: SUBTARGET := all_modules
all_modules: $(ALL_TARGETS)

$(ALL_TARGETS):
	make -C $@ $(SUBTARGET)

