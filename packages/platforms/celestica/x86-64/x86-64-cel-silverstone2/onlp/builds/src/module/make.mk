###############################################################################
#
# 
#
###############################################################################
THIS_DIR := $(dir $(lastword $(MAKEFILE_LIST)))
x86_64_cel_silverstone2_INCLUDES := -I $(THIS_DIR)inc
x86_64_cel_silverstone2_INTERNAL_INCLUDES := -I $(THIS_DIR)src
x86_64_cel_silverstone2_DEPENDMODULE_ENTRIES := init:x86_64_cel_silverstone2 ucli:x86_64_cel_silverstone2

