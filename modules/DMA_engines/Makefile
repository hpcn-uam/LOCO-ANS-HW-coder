


MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME :=

PART := pynq_z2
.PHONY: clean all test

ALL_TARGETS = idma odma odma_VarSize loopback_fifo idma_TSG
BASIC_TARGETS = idma odma idma_TSG

all: $(BASIC_TARGETS)

all_modules: $(ALL_TARGETS)


test: MODE := 0
test: all

$(ALL_TARGETS):
	make $@ MODE=$(MODE) PART=$(PART) -f submake.mk


clean:
	make -f submake.mk clean
