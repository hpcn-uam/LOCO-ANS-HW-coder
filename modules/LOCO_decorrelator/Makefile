
MODULE := LOCO_decorrelator
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

MODE := 1 # 1: just hls synth and export | 0: also csim and cosim
MODE_NAME := 
sources :=  src/$(MODULE).cpp 
test_sources :=  src/$(MODULE)_test.cpp src/sw_implementation.cpp src/sw_implementation.hpp
DEPS := $(sources) ../coder_config.hpp $(wildcard src/*.hpp) 


all: $(MODULE)

all_modules: all 

.PHONY: clean  distclean all $(MODULE)


$(MODULE): hls_synth export_ip

test: csim hls_synth cosim export_ip


#generic rules
csim: $(MODULE).hls_prj/solution1/csim 

hls_synth: $(MODULE).hls_prj/solution1/syn

cosim: $(MODULE).hls_prj/solution1/sim

export_ip: $(MODULE).hls_prj/solution1/impl/ip/

logs:
	mkdir -p $@

#csim 
%.hls_prj/solution1/csim: logs $(DEPS) $(test_sources)
	@echo  "$(GREEN) #########  C simulation: $* #########$(NC)"
	vitis_hls script.tcl 0 > logs/$*_csim.log

#synthesis
%.hls_prj/solution1/syn: logs $(DEPS)
	@echo  "$(GREEN) #########  HLS synthesis: $* #########$(NC)"
	vitis_hls script.tcl 1 > logs/$*_syn.log

#cosim 
%.hls_prj/solution1/sim: logs $(DEPS) $(test_sources)
	@echo  "$(GREEN) #########  RTL co-simulation: $* #########$(NC)"
	vitis_hls script.tcl 2 > logs/$*_cosim.log

#export
%.hls_prj/solution1/impl/ip/: logs %.hls_prj/solution1/syn  
	@echo  "$(GREEN) #########  Export synthesis: $* #########$(NC)"
	vitis_hls script.tcl 3 > logs/$*_export.log

clean:
	rm -rf logs vitis_hls*.tcl 

distclean: clean
	rm -rf *.hls_prj