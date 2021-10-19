
# CC:=clang++
CC:=g++

CFLAGS = -Wall -std=c++14 $(sources) `pkg-config --cflags  opencv`

all: debug

LIBS := -lopencv_highgui -lopencv_imgcodecs -lopencv_core -lopencv_imgproc
sources :=  $(wildcard src/*.cc)
deps  := $(wildcard src/*.cc) $(wildcard src/*.h) $(wildcard src/ANS_tables/*.dat )



.PHONY: debug release  profiling profiling_gprof
debug: CFLAGS += -g -DDEBUG -DANALYSIS_CODE
debug: loco_ans_codec

release: CFLAGS += -DNDEBUG -O3
release: loco_ans_codec

profiling_gprof: CFLAGS += -pg -DNDEBUG -O3
profiling_gprof:loco_ans_codec

profiling: CFLAGS += -g -DNDEBUG -O3
profiling: loco_ans_codec

loco_ans_codec: $(deps)
	$(CC) $(CFLAGS)  -o "$@"  `pkg-config  --libs  opencv`



ARM_RFS :=/home/tobi/Documents/Digital_design/Proyectos/HLS_Design/Linux/Pynq_system/RFS_partition
ARM_SYSROOT :=/home/tobi/Storage/tools/Vitis/2020.2/gnu/aarch32/lin/gcc-arm-linux-gnueabi
.PHONY: arm
arm: CFLAGS := -I$(ARM_RFS)/usr/include
arm: CFLAGS += -DNDEBUG -O3
# arm: CFLAGS += -march=armv7-a -mcpu=cortex-a9 -mfloat-abi=hard
arm: CLANG_FLAGS := -target arm-linux-gnueabihf  
# arm: CLANG_FLAGS := -target armv7a-xilinx-linux-eabi 
arm: CLANG_FLAGS += -mcpu=cortex-a9 -mfpu=NEON -mfloat-abi=hard
# arm: CLANG_FLAGS += -gcc-toolchain $(ARM_SYSROOT)
# arm: CLANG_FLAGS += --sysroot $(ARM_SYSROOT)
# arm: CLANG_FLAGS += --sysroot=/usr/arm-linux-gnueabihf
arm: CLANG_FLAGS += --sysroot=$(ARM_RFS)
# arm: CC := arm-linux-gnueabihf-g++
arm: loco_ans_codec_arm
# arm: LIBS_PATH += -L$(ARM_RFS)/lib/arm-linux-gnueabihf
# arm: LIBS_PATH := -L$(ARM_RFS)/usr/lib/arm-linux-gnueabihf
# arm: LIBS_PATH += -L$(ARM_RFS)/usr/lib/gcc/arm-linux-gnueabihf/5

loco_ans_codec_arm: $(deps)
	$(CC) $(CLANG_FLAGS) $(CFLAGS)  -o "$@" $(LIBS)  $(LIBS_PATH)

clean:
	rm -f loco_ans_codec
