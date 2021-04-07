LIBGO_PORT_DIR := $(call select_from_ports,libgo)

# place for build go packages to be given for any compilation via -I
LIBGO_PKG_BUILD := $(BUILD_BASE_DIR)/lib/libgo

# add includes from build for any .go compilation
CUSTOM_GO_FLAGS = -I$(LIBGO_PKG_BUILD)

# additional static libraries to link gccgo executables
LD_LIBGCC = \
${LIB_CACHE_DIR}/base-$(KERNEL)-common/base-$(KERNEL)-common.lib.a \
${LIB_CACHE_DIR}/startup-$(KERNEL)/startup-$(KERNEL).lib.a \
${LIB_CACHE_DIR}/cxx/cxx.lib.a \
${LIB_CACHE_DIR}/libc-stdlib/libc-stdlib.lib.a \
${LIB_CACHE_DIR}/libc-gen/libc-gen.lib.a  \
${BUILD_BASE_DIR}/lib/libgo/libgobegin.a \
${BUILD_BASE_DIR}/lib/libgo/libgolibbegin.a \
${BUILD_BASE_DIR}/lib/libgo/.libs/libgo.a
