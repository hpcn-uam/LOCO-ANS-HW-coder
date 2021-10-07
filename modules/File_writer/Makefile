
MODULE := File_writer
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

#parts:
# 0 - pynq z2
# 1 - zcu104
PART=0

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME :=
DEPS := $(wildcard src/*.cpp) $(wildcard src/*.hpp) ../coder_config.hpp

.PHONY: clean all

all_modules: all

all: $(MODULE)

test: MODE := 0
test: MODE_NAME := (with csim and RTL cosim)
test: $(MODULE)

$(MODULE): $(MODULE).hls_prj/solution1/impl/ip/

logs:
	mkdir -p $@

$(MODULE).hls_prj/solution1/impl/ip/: logs $(DEPS) script.tcl
	@echo  "$(GREEN) #########  Compiling $(MODULE) $(MODE_NAME) #########$(NC)"
	vitis_hls script.tcl $(MODE) $(PART) > logs/$(MODULE)_$(PART)_compile.log

clean:
	rm -rf $(MODULE).hls_prj vitis_hls*.tcl
