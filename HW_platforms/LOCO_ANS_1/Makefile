
RED=\e[31m
GREEN=\e[32m
NC=\e[0m

BOARDS= pynq_z2

all:  pynq_z2

### generic rules
.PHONY: all clean distclean $(BOARDS)

$(BOARDS): %: %_output_products/platform.xsa %_output_products/platform.bit %_output_products/platform.hwh %_output_products/platform.tcl
	@echo  "$(GREEN) #########  $@ Successfully built #########$(NC)"
	@echo  "$(GREEN) $*_output_products folder contains files to use platform with baremetal sw or with PYNQ  $(NC)"

.PRECIOUS: %_output_products vivado_%/platform.xsa

vivado_%/platform.xsa: bd_scripts/%_platform.tcl create_project.tcl 
	@echo  "$(GREEN) #########  Building $@ #########$(NC)"
	bash ./build_platform.sh $*

%_output_products:
	mkdir -p $@

%_output_products/platform.xsa: %_output_products vivado_%/platform.xsa
	cp vivado_$*/platform.xsa $@

%_output_products/platform.bit: %_output_products/platform.xsa
	unzip -oq $*_output_products/platform.xsa platform.bit -d $*_output_products

%_output_products/platform.hwh: %_output_products/platform.xsa
	unzip -oq $*_output_products/platform.xsa platform.hwh -d $*_output_products

%_output_products/platform.tcl: bd_scripts/%_platform.tcl
	cp $< $@

clean:
	rm -rf vivado_*

distclean: clean
	rm -rf *_output_products *.log *.jou