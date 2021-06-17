cmake_minimum_required(VERSION 3.8.2)
set_property(GLOBAL PROPERTY CSTD c11)


include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)
project(otg2)

enable_language(C ASM)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
 -DARM_MATH_CM4 \
 -DDISABLE_FLOAT_API \
 -DFIXED_POINT \
 -DUSE_ALLOCA \
 -DHAVE_LRINT \
 -DHAVE_LRINTF \
 -DOPUS_ARM_ASM \
 -DOPUS_ARM_INLINE_EDSP \
 -DOPUS_ARM_INLINE_MEDIA \
 -DOPUS_ARM_MAY_HAVE_EDSP \
 -DOPUS_ARM_PRESUME_EDSP \
 -DCUSTOM_SUPPORT \
 -DVAR_ARRAYS \
 -DOPUS_BUILD \
 -DHAVE_CONFIG_H \
 -D_errata_191_ \
 -g \
")
# -DNOGUI \
# -DOVERRIDE_OPUS_ALLOC -DOVERRIDE_OPUS_FREE -D'opus_alloc(x)=NULL' -D'opus_free(x)=NULL' \

# Use DOPUS_ARM_INLINE_EDSP instead of
# -DOPUS_ARM_INLINE_ASM \

# Not if use minimal lib
# -DHAVE_ALLOCA_H \



target_sources(                     app PRIVATE src/main.c)
target_sources(                     app PRIVATE src/clock.c)
target_sources(                     app PRIVATE src/user_led.c)


if(DEFINED CONFIG_AUDIO_CODEC_SGTL5000)
	FILE(GLOB_RECURSE audio_sources src/audio/*.c)
	FILE(GLOB_RECURSE radio_sources src/radio/*.c)
endif()


if(DEFINED CONFIG_DISPLAY)
	FILE(GLOB_RECURSE display_sources src/display/*.c)
	FILE(GLOB_RECURSE ihm_sources src/gui/*.c)
endif()

if(DEFINED CONFIG_FILE_SYSTEM)
	FILE(GLOB_RECURSE storage_sources src/storage/*.c)
endif()

FILE(GLOB_RECURSE stat_sources src/statistic/*.c)


FILE(GLOB_RECURSE opus_celt lib/opus-1.2.1/celt/*.c)
FILE(GLOB_RECURSE opus_src lib/opus-1.2.1/src/*.c)
target_sources(app PRIVATE
	${opus_celt}
	${opus_src}
	lib/opus-1.2.1/celt/arm/celt_pitch_xcorr_arm_gcc.s
	lib/opus-1.2.1/silk/biquad_alt.c
	lib/opus-1.2.1/silk/lin2log.c
	lib/opus-1.2.1/silk/log2lin.c
	)

FILE(GLOB_RECURSE nordic_source modules/hal/nordic/nrfx/drivers/src/*.c)


target_sources(app PRIVATE
	${audio_sources}
	${radio_sources}
	${display_sources}
	${ihm_sources}
	${storage_sources}
	${stat_sources}
	${nordic_source}
	)

zephyr_include_directories(
	src/
	src/audio/
	lib/opus-1.2.1/include
	lib/opus-1.2.1/silk/	
	lib/opus-1.2.1/silk/fixed
	lib/opus-1.2.1/celt
	lib/opus-1.2.1/src
	src/audio/sgtl5000/
	src/radio/
	src/display/
	src/gui/
	src/storage/
	src/stat/
	)
	
	
	
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
# -DUSE_LV_DEMO \
#")
