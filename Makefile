
RED=\e[31m
GREEN=\e[32m
NC=\e[0m
TARGET_BOARD = pynq_z2

TARGETS = TSG_coder LOCO_ANS_double_lane_1
.PHONY:	all  $(TARGETS) TSG_coder_hw_platform basic_modules

all: LOCO_ANS_double_lane_1 

BOARD=pynq_z2
PART=$(BOARD)


$(TARGETS): %: basic_modules HW_platforms/%/$(BOARD)_output_products/platform.xsa


HW_platforms/%/$(BOARD)_output_products/platform.xsa: basic_modules
	@echo  "$(GREEN) #########  Building $@ #########$(NC)"
	make -C HW_platforms/$* $(BOARD)

basic_modules:
	@echo  "$(GREEN) #########  Building $@ #########$(NC)"
	make -C modules -j4 PART=$(PART)


clean:
	make -C modules clean
# 	make -C HW_platforms clean