
MODULE := subsym_gen
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 
DEPS := $(wildcard src/*.*pp) $(wildcard ../input_buffers/src/input_buffers.*pp) ../coder_config.hpp

.PHONY: clean all

ALL_TARGETS := subsym_gen split_stream z_decompose_pre z_decompose_post serialize_symbols
BASIC_TARGETS := subsym_gen 

all: $(BASIC_TARGETS)
	
all_modules: $(ALL_TARGETS)


test: MODE := 0
test: MODE_NAME := (with csim and RTL cosim)
test: $(BASIC_TARGETS)

subsym_gen: subsym_gen.hls_prj/solution1/impl/ip/

split_stream: split_stream.hls_prj/solution1/impl/ip/

z_decompose_pre: z_decompose_pre.hls_prj/solution1/impl/ip/

z_decompose_post: z_decompose_post.hls_prj/solution1/impl/ip/

serialize_symbols: serialize_symbols.hls_prj/solution1/impl/ip/


%.hls_prj/solution1/impl/ip/: $(DEPS) %_script.tcl
	@echo  "$(GREEN) #########  Compiling $* $(MODE_NAME) #########$(NC)"
	vitis_hls $*_script.tcl $(MODE) > $*_compile.log

clean:
	rm -rf *.hls_prj vitis_hls*.tcl