

CFLAGS := -I/home/tobi/Storage/tools/Vitis_HLS/2020.2/include/
# CFLAGS+= -I/home/tobi/NVME/Xilinx/Vitis_HLS/2020.2/include/

sources :=  modules/input_buffers/src/input_buffers.cpp modules/subsym_gen/src/subsym_gen.cpp modules/ANS_coder/src/ANS_coder.cpp src/TSG_coder.cpp src/test.cpp
deps  := $(sources) $(wildcard src/*.hpp) $(wildcard modules/ANS_tables/*.dat )

TSG_CODER_csim: $(sources) $(deps)
	clang++ $(CFLAGS) $(sources)  -o TSG_CODER_csim

.PHONY: clean
clean:
	rm TSG_CODER_csim