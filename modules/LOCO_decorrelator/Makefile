
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 
sources :=  src/LOCO_decorrelator.cpp 
test_sources :=  src/LOCO_decorrelator_test.cpp src/sw_implementation.cpp src/sw_implementation.hpp
DEPS := $(sources) ../coder_config.hpp $(wildcard src/*.hpp) 

# TARGETS := LOCO_decorrelator
TARGETS := LOCO_decorrelator St_idx_compute
TEST_TARGETS := $(addsuffix _test, $(TARGETS))

.NOTPARALLEL:
	
all: $(TARGETS)

all_modules: all 

test: $(TEST_TARGETS)

complete: $(TEST_TARGETS) $(TARGETS) 


.PHONY: clean all $(TARGETS)

$(TARGETS): %: %.hls_prj/solution1/syn  %.hls_prj/solution1/impl/ip/
# $(MODULE):  hls_synth export_ip

$(TEST_TARGETS): %_test: %_script.tcl %.hls_prj/solution1/csim %.hls_prj/solution1/sim


#generic rules

logs:
	mkdir -p $@

#csim 
%.hls_prj/solution1/csim: %_script.tcl logs $(DEPS) $(test_sources)
	@echo  "$(GREEN) #########  C simulation: $* #########$(NC)"
	vitis_hls $*_script.tcl 0 > logs/$*_csim.log

#synthesis
%.hls_prj/solution1/syn: %_script.tcl logs $(DEPS)
	@echo  "$(GREEN) #########  HLS synthesis: $* #########$(NC)"
	vitis_hls $*_script.tcl 1 > logs/$*_syn.log

#cosim 
%.hls_prj/solution1/sim: %_script.tcl logs %.hls_prj/solution1/syn $(DEPS) $(test_sources)
	@echo  "$(GREEN) #########  RTL co-simulation: $* #########$(NC)"
	vitis_hls $*_script.tcl 2 > logs/$*_cosim.log

#export
%.hls_prj/solution1/impl/ip/: %_script.tcl logs %.hls_prj/solution1/syn  
	@echo  "$(GREEN) #########  Export IP : $* #########$(NC)"
	vitis_hls $*_script.tcl 3 > logs/$*_export.log



CFLAGS := -I/tools/Xilinx/Vitis_HLS/2021.1/include/ `pkg-config --cflags  opencv`
clang_sim: $(sources) $(test_sources)
	clang++ $(sources) src/LOCO_decorrelator_test.cpp src/sw_implementation.cpp  $(CFLAGS) `pkg-config --libs  opencv`  -o $@ 
# 	clang++ $(sources) src/LOCO_decorrelator_test.cpp src/sw_implementation.cpp  $(CFLAGS) `pkg-config --libs  opencv`  -o $@ --verbose

clean:
	rm -rf logs *.log *.hls_prj  clang_sim
