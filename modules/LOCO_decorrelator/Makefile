
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

#parts:
# 0 - pynq z2
# 1 - zcu104
PART=0

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME :=
sources :=  src/LOCO_decorrelator.cpp
test_sources :=  src/LOCO_decorrelator_test.cpp src/sw_implementation.cpp src/sw_implementation.hpp
DEPS := $(sources) ../coder_config.hpp $(wildcard src/*.hpp)

# TARGETS := LOCO_decorrelator
TARGETS := LOCO_decorrelator LOCO_decorrelator_LS St_idx_compute
TEST_TARGETS := $(addsuffix _test, $(TARGETS))

.NOTPARALLEL:

all: $(TARGETS)

all_modules: all

test: $(TEST_TARGETS)

complete: $(TEST_TARGETS) $(TARGETS)


.PHONY: clean all $(TARGETS)

$(TARGETS): %: %.hls_prj/$(PART)/syn  %.hls_prj/$(PART)/impl/ip/
# $(MODULE):  hls_synth export_ip

$(TEST_TARGETS): %_test: %_script.tcl %.hls_prj/$(PART)/csim %.hls_prj/$(PART)/sim


#generic rules

logs:
	mkdir -p $@

#csim
%.hls_prj/$(PART)/csim: %_script.tcl logs $(DEPS) $(test_sources)
	@echo  "$(GREEN) #########  C simulation: $* #########$(NC)"
	vitis_hls $*_script.tcl 0 $(PART) > logs/$*_$(PART)_csim.log

#synthesis
%.hls_prj/$(PART)/syn: %_script.tcl logs $(DEPS)
	@echo  "$(GREEN) #########  HLS synthesis: $* #########$(NC)"
	vitis_hls $*_script.tcl 1 $(PART) > logs/$*_$(PART)_syn.log

#cosim
%.hls_prj/$(PART)/sim: %_script.tcl logs %.hls_prj/$(PART)/syn $(DEPS) $(test_sources)
	@echo  "$(GREEN) #########  RTL co-simulation: $* #########$(NC)"
	vitis_hls $*_script.tcl 2 $(PART) > logs/$*_$(PART)_cosim.log

#export
%.hls_prj/$(PART)/impl/ip/: %_script.tcl logs %.hls_prj/$(PART)/syn
	@echo  "$(GREEN) #########  Export IP : $* #########$(NC)"
	vitis_hls $*_script.tcl 3 $(PART) > logs/$*_$(PART)_export.log



CFLAGS := -I/tools/Xilinx/Vitis_HLS/2021.1/include/ `pkg-config --cflags  opencv`
clang_sim: $(sources) $(test_sources)
	clang++ $(sources) src/LOCO_decorrelator_test.cpp src/sw_implementation.cpp  $(CFLAGS) `pkg-config --libs  opencv`  -o $@
# 	clang++ $(sources) src/LOCO_decorrelator_test.cpp src/sw_implementation.cpp  $(CFLAGS) `pkg-config --libs  opencv`  -o $@ --verbose

clean:
	rm -rf logs *.log *.hls_prj  clang_sim
