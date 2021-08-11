

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
DEPS := $(wildcard src/*.cpp) $(wildcard src/*.hpp) ../coder_config.hpp
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

TARGETS =  idma  odma odma_VarSize loopback_fifo idma_TSG
.PHONY: clean  test $(TARGETS)


$(TARGETS): %: %.hls_prj/solution1/impl/ip/

%.hls_prj/solution1/impl/ip/: $(DEPS) %_script.tcl
ifeq ($(MODE),0)
	@echo  "$(GREEN) #########  Compiling $* (with csim and RTL cosim) #########$(NC)"
else
	@echo  "$(GREEN) #########  Compiling $* #########$(NC)"
endif
	vitis_hls $*_script.tcl $(MODE) > $*_compile.log


clean:
	rm -rf *.hls_prj vitis_hls*.log *_compile.log
