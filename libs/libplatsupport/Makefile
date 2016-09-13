#
# Copyright 2014, NICTA
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(NICTA_BSD)
#

# For some reason, libplatsupport can't be built with make -j
.NOTPARALLEL:

# Targets
TARGETS := libplatsupport.a

CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/mach/$(MACH)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/arch/$(ARCH)/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/plat/$(PLAT)/acpi/*.c))
CFILES += $(patsubst $(SOURCE_DIR)/%,%,$(wildcard ${SOURCE_DIR}/src/*.c))

INCLUDE_DIRS += $(SOURCE_DIR)/plat_include/$(PLAT)

# Header files/directories this library provides
HDRFILES := $(wildcard ${SOURCE_DIR}/include/*) \
            $(wildcard ${SOURCE_DIR}/plat_include/$(PLAT)/*) \
            $(wildcard ${SOURCE_DIR}/arch_include/$(ARCH)/*)

ifneq ($(MACH),)
HDRFILES += $(wildcard ${SOURCE_DIR}/mach_include/$(MACH)/*)
endif

include $(SEL4_COMMON)/common.mk

CFLAGS += -flto
AR := $(CC)-ar
