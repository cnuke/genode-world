TARGET = test-go
SRC_GO = main.go

LIBS   = base stdcxx libc libm libgo libgo_support

# add place where compiled packages appears
CUSTOM_GO = /usr/local/genode/tool/19.05/bin/genode-x86-gccgo -B$(BUILD_BASE_DIR)/lib/libgo

TOOL_PATH:=$(dir $(GOC))

LD_CMD = /usr/local/genode/tool/19.05/bin/genode-x86-gcc -B ${TOOL_PATH}/x86/gcc/gcc/

# libraries to be add in the end of link/ld command
# order is important; problem in correct initializers order for different so libs for Env
LD_LIBGCC = \
${LIB_CACHE_DIR}/base-$(KERNEL)-common/base-$(KERNEL)-common.lib.a \
${LIB_CACHE_DIR}/startup-$(KERNEL)/startup-$(KERNEL).lib.a \
${LIB_CACHE_DIR}/cxx/cxx.lib.a \
${LIB_CACHE_DIR}/libc-stdlib/libc-stdlib.lib.a \
${LIB_CACHE_DIR}/libc-gen/libc-gen.lib.a  \
${BUILD_BASE_DIR}/lib/libgo/libgobegin.a \
${BUILD_BASE_DIR}/lib/libgo/libgolibbegin.a \
${BUILD_BASE_DIR}/lib/libgo/.libs/libgo.a \
/usr/local/genode/tool/current/lib/gcc/x86_64-pc-elf/8.3.0/64/libgcc_eh.a \
/usr/local/genode/tool/current/lib/gcc/x86_64-pc-elf/8.3.0/64/libgcc.a

CC_CXX_WARN_STRICT =
