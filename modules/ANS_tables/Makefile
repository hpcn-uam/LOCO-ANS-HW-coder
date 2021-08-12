

RED=\e[31m
GREEN=\e[32m
NC=\e[0m

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 
DEPS := $(wildcard src/*.*pp) $(wildcard ../input_buffers/src/input_buffers.*pp) ../coder_config.hpp

.PHONY: clean all

BASIC_TARGETS := Z_ANS_ROM Y_ANS_ROM
ALL_TARGETS := $(BASIC_TARGETS)

all: $(BASIC_TARGETS)
	
all_modules: $(ALL_TARGETS)

$(ALL_TARGETS): %:  %.hls_prj/solution1/impl/ip/


logs:
	mkdir -p logs

%.hls_prj/solution1/impl/ip/: $(DEPS) %_script.tcl logs
	@echo  "$(GREEN) #########  Compiling $* $(MODE_NAME) #########$(NC)"
	vitis_hls $*_script.tcl $(MODE) > logs/$*_compile.log

clean:
	rm -rf *.hls_prj vitis_hls*.tcl