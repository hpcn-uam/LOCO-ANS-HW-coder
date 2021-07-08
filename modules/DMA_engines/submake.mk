

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 
DEPS := $(wildcard src/*.cpp) $(wildcard src/*.hpp) ../coder_config.hpp
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

.PHONY: clean  test idma  odma odma_VarSize loopback_fifo


idma: MODULE = idma
idma: idma.hls_prj/solution1/impl/ip/

odma: MODULE = odma
odma: odma.hls_prj/solution1/impl/ip/

odma_VarSize: MODULE = odma_VarSize
odma_VarSize: odma_VarSize.hls_prj/solution1/impl/ip/

loopback_fifo: MODULE = loopback_fifo
loopback_fifo: loopback_fifo.hls_prj/solution1/impl/ip/


$(MODULE).hls_prj/solution1/impl/ip/: $(DEPS)
ifeq ($(MODE),0)
	@echo  "$(GREEN) #########  Compiling $(MODULE) (with csim and RTL cosim) #########$(NC)"
else
	@echo  "$(GREEN) #########  Compiling $(MODULE) #########$(NC)"
endif
	vitis_hls $(MODULE)_script.tcl $(MODE) > $(MODULE)_compile.log




clean:
	rm -rf *.hls_prj vitis_hls*.log *_compile.log