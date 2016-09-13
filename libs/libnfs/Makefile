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
TARGETS := libnfs.a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))
CXXFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.cpp))

# Header files/directories this library provides
RHDRFILES := '$(wildcard $(SOURCE_DIR)/include/*) nfs'

include $(SEL4_COMMON)/common.mk

CXXFLAGS += -DBOOST_THREAD_VERSION=4 -DBOOST_SYSTEM_NO_DEPRECATED -DBOOST_THREAD_PROVIDES_EXECUTORS

CFLAGS += -flto
CXXFLAGS += -flto
AR := $(CC)-ar

.PHONY: docs
docs: docs/libnfs.pdf

.PHONY: docs/libnfs.pdf
docs/libnfs.pdf: docs/Doxyfile include/nfs.h
	doxygen docs/Doxyfile
	$(MAKE) --directory=docs/latex
	cp docs/latex/refman.pdf docs/libnfs.pdf
