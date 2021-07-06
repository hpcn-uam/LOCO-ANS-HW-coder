


MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
DEPS := $(wildcard src/*.cc) $(wildcard src/*.hpp) ../coder_config.hpp


.PHONY: clean all test


TARGETS := idma odma_VarSize 
all: $(TARGETS)

test: MODE := 0
test: all

idma: idma.hls_prj/solution1/impl/ip/

idma.hls_prj/solution1/impl/ip/: $(DEPS)
ifeq ($(MODE),0)
	@echo "Compiling $(MODULE) with csim and RTL cosim"
else
	@echo "Compiling $(MODULE) "
endif
	vitis_hls idma_script.tcl $(MODE)


odma_VarSize: odma_VarSize.hls_prj/solution1/impl/ip/
	
odma_VarSize.hls_prj/solution1/impl/ip/: $(DEPS)
ifeq ($(MODE),0)
	@echo Compiling $(MODULE) with csim and RTL cosim
else
	@echo Compiling $(MODULE) 
endif
	vitis_hls odma_VarSize_script.tcl $(MODE)


loopback_fifo: loopback_fifo.hls_prj/solution1/impl/ip/
	
loopback_fifo.hls_prj/solution1/impl/ip/: $(DEPS)
ifeq ($(MODE),0)
	@echo "Compiling $(MODULE) with csim and RTL cosim"
else
	@echo "Compiling $(MODULE) "
endif
	vitis_hls loopback_fifo_script.tcl $(MODE)



clean:
	rm -rf *.hls_prj vitis_hls*.log