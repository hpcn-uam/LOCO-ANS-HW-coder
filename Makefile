
RED=\e[31m
GREEN=\e[32m
NC=\e[0m
TARGET_BOARD = pynq_z2

TARGETS = TSG_coder 
.PHONY:	all  $(TARGETS) TSG_coder_hw_platform basic_modules

all: TSG_coder


$(TARGETS): %: basic_modules %_hw_platform/$(TARGET_BOARD)_output_products/platform.xsa


%_hw_platform/$(TARGET_BOARD)_output_products/platform.xsa: basic_modules
	@echo  "$(GREEN) #########  Building $@ #########$(NC)"
	make -C HW_platforms/$* $(TARGET_BOARD)

basic_modules:
	@echo  "$(GREEN) #########  Building $@ #########$(NC)"
	make -C modules -j4


clean:
	make -C modules clean
# 	make -C HW_platforms clean