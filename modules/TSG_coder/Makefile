
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

CFLAGS := -I/tools/Xilinx/Vitis_HLS/2021.1/include/
# CFLAGS+= -I/home/tobi/NVME/Xilinx/Vitis_HLS/2021.1/include/

sources :=  src/TSG_coder.cpp  ../input_buffers/src/input_buffers.cpp ../subsym_gen/src/subsym_gen.cpp ../ANS_coder/src/ANS_coder.cpp ../output_stack/src/output_stack.cpp
# deps  := src/TSG_coder.cpp $(wildcard src/*.hpp) $(wildcard ../ANS_tables/*.dat )

#parts:
# 0 - pynq z2
# 1 - zcu104
PART= pynq_z2

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME :=
DEPS :=   $(wildcard src/*.hpp) $(wildcard ../ANS_tables/*.dat ) ../user_config.hpp ../coder_config.hpp

.PHONY: clean all

ALL_TARGETS := TSG_coder TSG_coder_ext_ROM TSG_coder_double_lane TSG_input_adapter output_data_interface output_metadata_interface
BASIC_TARGETS := TSG_coder TSG_coder_double_lane


all: $(BASIC_TARGETS)

all_modules: $(ALL_TARGETS)

test: MODE := 0
test: MODE_NAME := (with csim and RTL cosim)
test: TSG_coder

$(ALL_TARGETS): %: logs %.hls_prj/$(PART)/impl/ip/


logs:
	mkdir -p logs

%.hls_prj/$(PART)/impl/ip/: $(DEPS) %_script.tcl
	@echo  "$(GREEN) #########  Compiling $* $(MODE_NAME) #########$(NC)"
	vitis_hls $*_script.tcl $(MODE) $(PART) > logs/$*_$(PART)_compile.log


TSG_csim_clang_sim: src/test.cpp $(sources)
	clang++ $(CFLAGS) $(sources)  $< -o $@


TSG_csim_clang_sim_external_ROM:  src/test.cpp $(sources)
	clang++ $(CFLAGS) -DDEBUG -DEXTERNAL_ANS_ROM $(sources) $< -o $@

TSG_csim_clang_sim_TSG_internal_ROM:  src/test.cpp $(sources)
	clang++ $(CFLAGS) -DDEBUG -DUSE_TSG_INTERNAL_ROM -DEXTERNAL_ANS_ROM $(sources) $< -o $@

clean:
	rm -rf TSG_csim_clang_sim *.hls_prj vitis_hls.log logs
