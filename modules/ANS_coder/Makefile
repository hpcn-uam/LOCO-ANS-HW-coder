
MODULE := ANS_coder
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 
sources :=  ../input_buffers/src/input_buffers.cpp ../subsym_gen/src/subsym_gen.cpp $(wildcard src/*.cpp)
DEPS := $(sources) $(wildcard src/*.hpp) $(wildcard ../input_buffers/src/*.hpp) $(wildcard ../subsym_gen/src/*.hpp)  $(wildcard ../ANS_tables/*.dat )

CFLAGS := -I/tools/Xilinx/Vitis_HLS/2020.2/include/

.PHONY: clean all run_sim $(MODULE)

all_modules: all 

all: $(MODULE)

test: MODE := 0
test: MODE_NAME := (with csim and RTL cosim)
test: $(MODULE)

$(MODULE): $(MODULE).hls_prj/solution1/impl/ip/

$(MODULE).hls_prj/solution1/impl/ip/: $(DEPS)
	@echo  "$(GREEN) #########  Compiling $(MODULE) $(MODE_NAME) #########$(NC)"
	vitis_hls script.tcl $(MODE) > $(MODULE)_compile.log

run_sim: ANS_coder_sim
	echo "Running simulation:"
	./ANS_coder_sim

ANS_coder_sim: $(sources) $(DEPS)
	clang++ $(CFLAGS) $(sources)  -o ANS_coder_sim

clean:
	rm -rf $(MODULE).hls_prj vitis_hls*.tcl ANS_coder_sim