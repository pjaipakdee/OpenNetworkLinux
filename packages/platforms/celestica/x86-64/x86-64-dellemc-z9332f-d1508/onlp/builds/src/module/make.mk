###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_dellemc_z9332f_d1508_INCLUDES := -I $(THIS_DIR)inc
x86_64_dellemc_z9332f_d1508_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_dellemc_z9332f_d1508_DEPENDMODULE_ENTRIES := init:x86_64_dellemc_z9332f_d1508 ucli:x86_64_dellemc_z9332f_d1508

