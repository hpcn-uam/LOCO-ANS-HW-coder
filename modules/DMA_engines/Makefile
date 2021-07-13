


MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 


.PHONY: clean all test

ALL_TARGETS = idma odma odma_VarSize loopback_fifo
BASIC_TARGETS = idma odma 

all: $(BASIC_TARGETS)
	
all_modules: $(ALL_TARGETS)


test: MODE := 0
test: all

$(ALL_TARGETS): 
	make $@ MODE=$(MODE) -f submake.mk


clean:
	make -f submake.mk clean