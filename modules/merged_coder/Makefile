
CFLAGS= -I./src/merged_coder.hpp
CFLAGS+= -I../../subsym_gen/src/subsym_gen.hpp
CFLAGS+= -I../../coder_config.hpp
CFLAGS+= -I~/NVME/Xilinx/Vitis_HLS/2020.2/include/
# CFLAGS+=" -I~/Storage/tools/Vitis_HLS/2020.2/include/"

test: src/merged_coder.cpp ../subsym_gen/src/subsym_gen.cpp
	clang++ -I/home/tobi/NVME/Xilinx/Vitis_HLS/2020.2/include/ -I../coder_config.hpp -I../input_buffers/src/subsym_gen.hpp -I../subsym_gen/src/subsym_gen.hpp -Isrc/merged_coder.hpp  ../input_buffers/src/input_buffers.cpp src/merged_coder.cpp src/test.cpp  -o test