#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# Targets
TARGETS := libclock.a

# Source files required to build the target
CXXFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.cpp))

HDRFILES := $(wildcard $(SOURCE_DIR)/include/*)

include $(SEL4_COMMON)/common.mk
