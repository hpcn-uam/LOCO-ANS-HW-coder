


CFLAGS := -I/home/tobi/Storage/tools/Vitis_HLS/2020.2/include/
# CFLAGS+= -I/home/tobi/NVME/Xilinx/Vitis_HLS/2020.2/include/

sources :=  ../input_buffers/src/input_buffers.cpp ../subsym_gen/src/subsym_gen.cpp src/ANS_coder.cpp  src/test.cpp
deps  := $(sources) $(wildcard src/*.hpp) $(wildcard ../ANS_tables/*.dat )

all: ANS_coder_sim run

run:
	echo "Running simulation:"
	./ANS_coder_sim

ANS_coder_sim: $(sources) $(deps)
	clang++ $(CFLAGS) $(sources)  -o ANS_coder_sim

.PHONY: clean
clean:
	rm ANS_coder_sim