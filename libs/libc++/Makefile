# Targets
TARGETS := libc++ libc++abi

include $(SEL4_COMMON)/common.mk

CPPFLAGS := $(CPPFLAGS) -I$(SOURCE_DIR)/libunwind/include
CFLAGS := $(CFLAGS) $(CPPFLAGS)
CXXFLAGS := $(filter-out -I$(STAGE_DIR)/include/c++/v1,$(CXXFLAGS) $(CPPFLAGS))

# cmake tries to link during config despite nothing needing the linker in the
# actual build, but causes problems due to missing libraries, so we ignore all
# unresolved symbols
LDFLAGS := -Wl,--unresolved-symbols=ignore-all

.PHONY: libc++ libc++abi

$(SOURCE_DIR)/llvm/CMakeLists.txt:
	mkdir -p "$(SOURCE_DIR)/llvm"
	# No HTTPS gosh :(
	curl "http://llvm.org/releases/3.8.1/llvm-3.8.1.src.tar.xz" | tar xJf - -C "$(SOURCE_DIR)/llvm" --strip-components=1

$(SOURCE_DIR)/libcxx/CMakeLists.txt: | $(SOURCE_DIR)/libcxx-sos.patch
	mkdir -p "$(SOURCE_DIR)/libcxx"
	curl "http://llvm.org/releases/3.8.1/libcxx-3.8.1.src.tar.xz" | tar xJf - -C "$(SOURCE_DIR)/libcxx" --strip-components=1
	(cd "$(SOURCE_DIR)/libcxx" && patch -mp1 < ../libcxx-sos.patch)

$(SOURCE_DIR)/libcxxabi/CMakeLists.txt: | $(SOURCE_DIR)/libcxxabi-sos.patch
	mkdir -p "$(SOURCE_DIR)/libcxxabi"
	curl "http://llvm.org/releases/3.8.1/libcxxabi-3.8.1.src.tar.xz" | tar xJf - -C "$(SOURCE_DIR)/libcxxabi" --strip-components=1
	(cd "$(SOURCE_DIR)/libcxxabi" && patch -mp1 < ../libcxxabi-sos.patch)

$(SOURCE_DIR)/libunwind/CMakeLists.txt:
	mkdir -p "$(SOURCE_DIR)/libunwind"
	curl "http://llvm.org/releases/3.8.1/libunwind-3.8.1.src.tar.xz" | tar xJf - -C "$(SOURCE_DIR)/libunwind" --strip-components=1

libc++: $(SOURCE_DIR)/llvm/CMakeLists.txt $(SOURCE_DIR)/libcxx/CMakeLists.txt $(SOURCE_DIR)/libcxxabi/CMakeLists.txt
	mkdir -p "$(BUILD_DIR)/libcxx"
	cd "$(BUILD_DIR)/libcxx" && \
		cmake \
			-DCMAKE_SYSTEM_NAME=Generic \
			-DCMAKE_SYSTEM_PROCESSOR=arm-eabi \
			-DCMAKE_C_COMPILER="$(strip $(CC))" \
			-DCMAKE_CXX_COMPILER="$(strip $(CXX))" \
			-DLLVM_PATH="$(SOURCE_DIR)/llvm" \
			-DLIBCXX_HAS_MUSL_LIBC=on \
			-DLIBCXX_ENABLE_THREADS=off \
			-DLIBCXX_CXX_ABI=libcxxabi \
			-DLIBCXX_CXX_ABI_INCLUDE_PATHS="$(SOURCE_DIR)/libcxxabi/include" \
			-DLIBCXX_ENABLE_STATIC_ABI_LIBRARY=on \
			-DLIBCXXABI_USE_LLVM_UNWINDER=off \
			-DLIBCXX_ENABLE_SHARED=off \
			-DCMAKE_INSTALL_PREFIX="$(STAGE_DIR)" \
			-DUNIX=1 \
			"$(SOURCE_DIR)/libcxx" && \
		$(MAKE) -j && $(MAKE) install

libc++abi: libc++ $(SOURCE_DIR)/llvm/CMakeLists.txt $(SOURCE_DIR)/libcxx/CMakeLists.txt $(SOURCE_DIR)/libcxxabi/CMakeLists.txt $(SOURCE_DIR)/libunwind/CMakeLists.txt
	mkdir -p "$(BUILD_DIR)/libcxxabi"
	cd "$(BUILD_DIR)/libcxxabi" && \
		cmake \
			-DCMAKE_SYSTEM_NAME=Generic \
			-DCMAKE_SYSTEM_PROCESSOR=arm-eabi \
			-DCMAKE_C_COMPILER="$(strip $(CC))" \
			-DCMAKE_CXX_COMPILER="$(strip $(CXX))" \
			-DLLVM_PATH="$(SOURCE_DIR)/llvm" \
			-DLIBCXXABI_LIBCXX_PATH="$(SOURCE_DIR)/libcxx" \
			-DLIBCXXABI_LIBUNWIND_PATH="$(SOURCE_DIR)/libunwind" \
			-DLIBCXXABI_USE_LLVM_UNWINDER=off \
			-DLIBCXXABI_ENABLE_SHARED=off \
			-DCMAKE_INSTALL_PREFIX="$(STAGE_DIR)" \
			-DUNIX=1 \
			-DLLVM_FORCE_USE_OLD_TOOLCHAIN=1 \
			"$(SOURCE_DIR)/libcxxabi" && \
		$(MAKE) -j && $(MAKE) install
