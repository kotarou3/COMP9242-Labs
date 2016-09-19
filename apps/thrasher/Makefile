TARGETS := thrasher.bin

CXXFILES   := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.cpp))

LIBS := c sel4 sos c++ c++abi
include $(SEL4_COMMON)/common.mk

CPPFLAGS += -Werror
