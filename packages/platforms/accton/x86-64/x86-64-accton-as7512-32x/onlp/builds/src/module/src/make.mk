###############################################################################
#
# 
#
###############################################################################

LIBRARY := x86_64_accton_as7512_32x
$(LIBRARY)_SUBDIR := $(dir $(lastword $(MAKEFILE_LIST)))
include $(BUILDER)/lib.mk