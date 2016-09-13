TARGETS := libboost

include $(SEL4_COMMON)/common.mk

CXXFLAGS += -DBOOST_THREAD_VERSION=4 -DBOOST_SYSTEM_NO_DEPRECATED

.PHONY: libboost

$(SOURCE_DIR)/boost/bootstrap.sh: | $(SOURCE_DIR)/sos.patch
	mkdir -p "$(SOURCE_DIR)/boost"
	curl -L "https://downloads.sourceforge.net/project/boost/boost/1.61.0/boost_1_61_0.tar.bz2" | tar xjf - -C "$(SOURCE_DIR)/boost" --strip-components=1
	(cd "$(SOURCE_DIR)/boost" && patch -mp1 < ../sos.patch)

$(BUILD_DIR)/project-config.jam: | $(SOURCE_DIR)/boost/bootstrap.sh
	mkdir -p "$(BUILD_DIR)"
	cp -al "$(SOURCE_DIR)/boost/." "$(BUILD_DIR)"
	cd "$(BUILD_DIR)" && \
		./bootstrap.sh --with-libraries=thread,system && \
		sed -i "s/using gcc ;/using gcc : arm : $(CXX) : <archiver>$(strip $(CC))-ar <ranlib>$(strip $(CC))-ranlib ;/" project-config.jam

libboost: $(BUILD_DIR)/project-config.jam
	cd "$(BUILD_DIR)" && \
		./bjam -j8 --prefix="$(STAGE_DIR)" link=static runtime-link=static toolset=gcc-arm cxxflags="$(CXXFLAGS) $(CPPFLAGS) -flto" install
