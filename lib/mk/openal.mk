LIBS       = libc stdcxx
SHARED_LIB = yes
AL_BASE    = $(call select_from_ports,openal)/src/lib/openal

INC_DIR = $(call select_from_ports,openal)/include \
          $(REP_DIR)/src/lib/openal/include \
          $(AL_BASE) $(AL_BASE)/common

SRC_CC = common/threads.cpp \
         common/strutils.cpp \
         common/ringbuffer.cpp \
         common/polyphase_resampler.cpp \
         common/dynload.cpp \
         common/alstring.cpp \
         common/almalloc.cpp \
         common/alcomplex.cpp \
         core/mixer/mixer_sse41.cpp \
         core/mixer/mixer_sse2.cpp \
         core/mixer/mixer_sse.cpp \
         core/mixer/mixer_c.cpp \
         core/voice.cpp \
         core/uhjfilter.cpp \
         core/mixer.cpp \
         core/mastering.cpp \
         core/logging.cpp \
         core/hrtf.cpp \
         core/helpers.cpp \
         core/fpu_ctrl.cpp \
         core/fmt_traits.cpp \
         core/filters/splitter.cpp \
         core/filters/nfc.cpp \
         core/filters/biquad.cpp \
         core/except.cpp \
         core/device.cpp \
         core/devformat.cpp \
         core/cpu_caps.cpp \
         core/converter.cpp \
         core/context.cpp \
         core/buffer_storage.cpp \
         core/bsinc_tables.cpp \
         core/bs2b.cpp \
         core/bformatdec.cpp \
         core/ambidefs.cpp \
         core/ambdec.cpp \
         alc/backends/oss.cpp \
         alc/backends/null.cpp \
         alc/backends/loopback.cpp \
         alc/backends/base.cpp \
         alc/panning.cpp \
         alc/effects/vmorpher.cpp \
         alc/effects/reverb.cpp \
         alc/effects/pshifter.cpp \
         alc/effects/null.cpp \
         alc/effects/modulator.cpp \
         alc/effects/fshifter.cpp \
         alc/effects/equalizer.cpp \
         alc/effects/echo.cpp \
         alc/effects/distortion.cpp \
         alc/effects/dedicated.cpp \
         alc/effects/convolution.cpp \
         alc/effects/compressor.cpp \
         alc/effects/chorus.cpp \
         alc/effects/autowah.cpp \
         alc/effectslot.cpp \
         alc/device.cpp \
         alc/context.cpp \
         alc/alconfig.cpp \
         alc/alu.cpp \
         alc/alc.cpp \
         al/state.cpp \
         al/source.cpp \
         al/listener.cpp \
         al/filter.cpp \
         al/extension.cpp \
         al/event.cpp \
         al/error.cpp \
         al/effects/vmorpher.cpp \
         al/effects/reverb.cpp \
         al/effects/pshifter.cpp \
         al/effects/null.cpp \
         al/effects/modulator.cpp \
         al/effects/fshifter.cpp \
         al/effects/equalizer.cpp \
         al/effects/echo.cpp \
         al/effects/distortion.cpp \
         al/effects/dedicated.cpp \
         al/effects/convolution.cpp \
         al/effects/compressor.cpp \
         al/effects/chorus.cpp \
         al/effects/autowah.cpp \
         al/effect.cpp \
         al/buffer.cpp \
         al/auxeffectslot.cpp \

SRC_S = tls.s

CC_OPT= -DAL_ALEXT_PROTOTYPES \
        -DAL_ALEXT_PROTOTYPES \
        -DAL_API="" \
        -DAL_BUILD_LIBRARY \
        -DOpenAL_EXPORTS \
        -DRESTRICT=__restrict



CC_CXX_WARN_STRICT =

vpath %.cpp $(AL_BASE)
vpath %.s $(REP_DIR)/src/lib/openal
