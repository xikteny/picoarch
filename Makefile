## Global definitions
# default to native Linux platform
platform      ?= unix
core_platform ?= $(platform)

# use cross-compilation if the environment has it defined
CC             = $(CROSS_COMPILE)gcc
SYSROOT        = $(shell $(CC) --print-sysroot)
CXX            = $(CROSS_COMPILE)g++
AR             = $(CROSS_COMPILE)ar

# use four threads when building
PROCS          = -j4

# base picoarch sources
SOURCES        = libpicofe/input.c libpicofe/in_sdl.c libpicofe/linux/in_evdev.c libpicofe/linux/plat.c libpicofe/fonts.c libpicofe/readpng.c libpicofe/config_file.c cheat.c config.c content.c core.c menu.c main.c options.c overrides.c patch.c rewind.c scale.c unzip.c util.c video.c

# name of picoarch binary
BIN            = picoarch

# Blank environmental compiler flags
unexport CFLAGS
CFLAGS :=
unexport CPPFLAGS
CPPFLAGS :=
unexport CXXFLAGS
CXXFLAGS :=
unexport LDFLAGS
LDFLAGS :=
unexport FCFLAGS
FCFLAGS :=
unexport FFLAGS
FFLAGS :=

# Base CFLAGS: warnings, includes, defines
CFLAGS        += -Wall
CFLAGS        += -fdata-sections -ffunction-sections -DPICO_HOME_DIR='"/.picoarch/"' -flto
CFLAGS        += -I./ -I./libretro-common/include/ $(shell $(SYSROOT)/usr/bin/sdl-config --cflags)
CFLAGS        += -MMD -MP

# Revision info from repository
GIT_REVISION  ?= $(shell git rev-parse --short HEAD || echo unknown)
CFLAGS        += -DREVISION=\"$(GIT_REVISION)\"

# base linker flags
LDFLAGS        = -lc -ldl -lgcc -lm -lSDL -lasound -lpng -lz -llz4 -lpthread -Wl,--gc-sections -flto

# Single core for testing purposes
CORES          = pokemini
# The full set of core settings are below, commented by "## "
## CORES          = bluemsx fbalpha2012 fceumm fmsx gambatte gme gpsp mame2000 mame2003_plus mednafen_pce_fast pcsx_rearmed picodrive quicknes smsplus-gx snes9x2002
## ifneq ($(platform), trimui)
## 	CORES      := $(CORES) chimerasnes dosbox-pure ecwolf fake-08 mednafen_lynx mednafen_ngp mednafen_wswan pcsx_rearmed pokemini prboom scummvm snes9x2005 snes9x2005_plus snes9x2010 stella2014 tyrquake vitaquake2
## endif

SOFILES        = $(foreach core,$(CORES),$(core)_libretro.so)
include Makefile.cores

.PHONY: print-%
print-%:
	@echo '$*=$($*)'

.PHONY: all
all: $(BIN) cores

define CORE_template =
    $1_REPO ?= https://github.com/libretro/$(1)/
    $1_BUILD_PATH ?= $(1)
    $1_LICENSE ?= LICENSE
    $1_MAKE = make $(and $($1_MAKEFILE),-f $($1_MAKEFILE)) platform=$(core_platform) $(and $(DEBUG),DEBUG=$(DEBUG)) $(and $(PROFILE),PROFILE=$(PROFILE)) CC=$(CC) CXX=$(CXX) AR=$(AR) $($(1)_FLAGS)
$(1):
	git clone $(if $($1_REVISION),,--depth 1) --recursive $$($(1)_REPO) $(1)
	$(if $1_REVISION,cd $(1) && git checkout $($1_REVISION),)
	(test ! -d patches/$(1)) || (cd $(1) && $(foreach patch, $(sort $(wildcard patches/$(1)/*.patch)), patch --merge --no-backup-if-mismatch -p1 < ../$(patch) &&) true)
$(1)/$(1)_libretro.so: $(1)
	cd $$($1_BUILD_PATH) && $$($1_MAKE) $(PROCS)
$(1)_libretro.so: $(1)/$(1)_libretro.so
	cp -v $$($1_BUILD_PATH)/$(if $($(1)_CORE),$($(1)_CORE),$(1)_libretro.so) $(1)_libretro.so
clean-$(1):
	test ! -d $(1) || (cd $$($1_BUILD_PATH) && $$($1_MAKE) clean)
	rm -fv $(1)_libretro.so
endef

$(foreach core,$(CORES),$(eval $(call CORE_template,$(core))))

# install_licenses: $(1)=destination dir, $(2)=core name(s) (optional)
define install_licenses
	mkdir -pv "$(1)/LICENSES"
	curl -L -o "$(1)/LICENSES/liblz4.txt" "https://raw.githubusercontent.com/lz4/lz4/refs/heads/dev/lib/LICENSE"
	cp -v "libpicofe/README" "$(1)/LICENSES/libpicofe.txt"
	cp -v "LICENSE" "$(1)/LICENSES/picoarch.txt"
	$(foreach core,$(2),cp -v "$(core)/$($(core)_LICENSE)" "$(1)/LICENSES/$(core)_libretro.txt";)
endef

# install_liblz4: $(1)=destination dir
define install_liblz4
	mkdir -pv "$(1)/lib"
	cp -Lv "$(SYSROOT)/usr/lib/liblz4.so.1" "$(1)/lib/"
endef

# Platform-specific SOURCES, CFLAGS, LDFLAGS, and dist targets
-include Makefile.$(platform)

# Debug/Profile CFLAGS
ifeq ($(DEBUG), 1)
    CFLAGS    += -Og -g
    LDFLAGS   += -g
else
    CFLAGS    += -Ofast -DNDEBUG

ifneq ($(PROFILE), 1)
    LDFLAGS   += -s
endif

endif

ifeq ($(PROFILE), 1)
    CFLAGS    += -fno-omit-frame-pointer -pg -g
    LDFLAGS   += -pg -g
else ifeq ($(PROFILE), GENERATE)
    CFLAGS    += -fprofile-generate=./profile/picoarch
    LDFLAGS   += -lgcov
else ifeq ($(PROFILE), APPLY)
    CFLAGS    += -fprofile-use -fprofile-dir=./profile/picoarch -fbranch-probabilities
endif

ifeq ($(MINUI), 1)
    MMENU      = 1
    CFLAGS    += -DMINUI
endif

ifeq ($(MMENU), 1)
    CFLAGS    += -DMMENU
    LDFLAGS   += -lSDL_image -lSDL_ttf -ldl
endif

CFLAGS        += $(EXTRA_CFLAGS)

libpicofe/.patched:
	cd libpicofe && ($(foreach patch, $(sort $(wildcard patches/libpicofe/*.patch)), patch --no-backup-if-mismatch --merge -p1 < ../$(patch) &&) touch .patched)

reverse = $(if $(wordlist 2,2,$(1)),$(call reverse,$(wordlist 2,$(words $(1)),$(1))) $(firstword $(1)),$(1))

.PHONY: clean-libpicofe
clean-libpicofe:
	test ! -f libpicofe/.patched || (cd libpicofe && ($(foreach patch, $(call reverse,$(sort $(wildcard patches/libpicofe/*.patch))), patch -R --merge --no-backup-if-mismatch -p1 < ../$(patch) &&) rm .patched))

DEPS=$(SOURCES:.c=.d)
$(DEPS):

include $(wildcard $(DEPS))

OBJS = $(SOURCES:.c=.o)

$(BIN): libpicofe/.patched $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $(BIN)

.PHONY: cores
cores: $(SOFILES)

.PHONY: clean-picoarch
clean-picoarch:
	rm -fv $(DEPS) $(OBJS) $(BIN)
	rm -rfv pkg
	rm -fv *.opk

.PHONY: clean
clean: clean-libpicofe clean-picoarch
	rm -fv $(SOFILES)

.PHONY: clean-all
clean-all: $(foreach core,$(CORES),clean-$(core)) clean

.PHONY: distclean
distclean: clean
	rm -rfv $(CORES) *.zip
