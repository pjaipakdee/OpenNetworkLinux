###############################################################################
#
# 
#
###############################################################################

LIBRARY := x86_64_delta_ag7648
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk