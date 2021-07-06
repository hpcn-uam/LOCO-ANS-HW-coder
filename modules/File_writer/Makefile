
MODULE := File_writer

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
DEPS := $(wildcard src/*.cc) $(wildcard src/*.h) $(wildcard src/ANS_tables/*.dat )

.PHONY: clean all

all: $(MODULE)

test: MODE := 0
test: $(MODULE)

$(MODULE): $(MODULE).hls_prj/solution1/impl/ip/

$(MODULE).hls_prj/solution1/impl/ip/: $(DEPS)
ifeq ($(MODE),0)
	@echo "Compiling $(MODULE) (with csim and RTL cosim)"
else
	@echo "Compiling $(MODULE) "
endif
	vitis_hls script.tcl $(MODE)

clean:
	rm -rf $(MODULE).hls_prj vitis_hls*.tcl