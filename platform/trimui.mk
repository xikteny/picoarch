SOURCES                   += plat_trimui.c
CFLAGS                    += -mcpu=arm926ej-s -mtune=arm926ej-s -fno-PIC -DCONTENT_DIR='"/mnt/SDCARD/Roms"'
LDFLAGS                   += -fno-PIC

bluemsx_NAME               = blueMSX
bluemsx_ROM_DIR            = MSX
bluemsx_PAK_NAME           = MSX (blueMSX)

fbalpha2012_NAME           = fba2012
fbalpha2012_ROM_DIR        = ARCADE
fbalpha2012_PAK_NAME       = Arcade (FBA)

fceumm_ROM_DIR             = FC
fceumm_PAK_NAME            = Nintendo (fceumm)

fmsx_NAME                  = fMSX
fmsx_ROM_DIR               = MSX
fmsx_PAK_NAME              = MSX

gambatte_ROM_DIR           = GB
gambatte_PAK_NAME          = Game Boy

gpsp_ROM_DIR               = GBA
gpsp_PAK_NAME              = Game Boy Advance
define gpsp_PAK_EXTRA
    needs-swap
endef

gme_ROM_DIR                = MUSIC
gme_PAK_NAME               = Game Music

mame2000_ROM_DIR           = ARCADE
mame2000_PAK_NAME          = Arcade

mame2003_plus_NAME         = mame2003+
mame2003_plus_ROM_DIR      = ARCADE
mame2003_plus_PAK_NAME     = Arcade (MAME 2003-plus)

mednafen_ngp_NAME          = ngp
mednafen_ngp_ROM_DIR       = NGP
mednafen_ngp_PAK_NAME      = Neo Geo Pocket

mednafen_pce_fast_NAME     = pce_fast
mednafen_pce_fast_ROM_DIR  = PCE
mednafen_pce_fast_PAK_NAME = TurboGrafx-16

mednafen_wswan_NAME        = wswan
mednafen_wswan_ROM_DIR     = WS
mednafen_wswan_PAK_NAME    = WonderSwan

picodrive_ROM_DIR          = MD
picodrive_PAK_NAME         = Genesis

pokemini_ROM_DIR           = POKEMINI
pokemini_PAK_NAME          = PokeMini

pcsx_rearmed_ROM_DIR       = PS
pcsx_rearmed_PAK_NAME      = PlayStation
define pcsx_rearmed_PAK_EXTRA
    needs-swap
endef

quicknes_ROM_DIR           = FC
quicknes_PAK_NAME          = Nintendo

smsplus-gx_ROM_DIR         = MS
smsplus-gx_PAK_NAME        = Game Gear

snes9x2002_ROM_DIR         = SFC
snes9x2002_PAK_NAME        = Super Nintendo

snes9x2005_ROM_DIR         = SFC
snes9x2005_PAK_NAME        = Super Nintendo (2005)

stella2014_ROM_DIR         = 2600
stella2014_PAK_NAME        = Atari 2600

.PHONY: dist-gmenu-section dist-gmenu-picoarch dist-gmenu dist-minui-picoarch dist-minui

# -- gmenunx

dist-gmenu-section:
	mkdir -pv pkg/gmenunx/Apps/picoarch
	mkdir -pv pkg/gmenunx/Apps/gmenunx/sections/emulators
	mkdir -pv pkg/gmenunx/Apps/gmenunx/sections/libretro
	touch pkg/gmenunx/Apps/gmenunx/sections/libretro/.section

dist-gmenu-picoarch: $(BIN) dist-gmenu-section
	cp -v $(BIN) pkg/gmenunx/Apps/picoarch
	$(file >pkg/gmenunx/Apps/picoarch/picoarch.sh,$(picoarch_LAUNCHER))
	$(call install_licenses,pkg/gmenunx/Apps/picoarch)
	$(call install_liblz4,pkg/gmenunx/Apps/picoarch)
	$(file >pkg/gmenunx/Apps/gmenunx/sections/emulators/picoarch,$(picoarch_SHORTCUT))

define CORE_gmenushortcut =

$1_NAME ?= $1

define $1_SHORTCUT
title=$$($1_NAME)
exec=/mnt/SDCARD/Apps/picoarch/picoarch.sh
params=/mnt/SDCARD/Apps/picoarch/$1_libretro.so
selectordir=/mnt/SDCARD/Roms/$($1_ROM_DIR)
selectorfilter=$($1_TYPES)
endef

.PHONY: dist-gmenu-$(1)
dist-gmenu-$(1): $(BIN) $(1)_libretro.so dist-gmenu-picoarch dist-gmenu-section
	cp -v $1_libretro.so pkg/gmenunx/Apps/picoarch
	cp -v $(1)/$($(1)_LICENSE) pkg/gmenunx/Apps/picoarch/LICENSES/$(1)_libretro.txt
	$$(file >pkg/gmenunx/Apps/gmenunx/sections/libretro/$(1),$$($(1)_SHORTCUT))

endef

$(foreach core, $(CORES),$(eval $(call CORE_gmenushortcut,$(core))))

define picoarch_SHORTCUT
title=$(BIN)
exec=/mnt/SDCARD/Apps/picoarch/picoarch.sh
endef

define picoarch_LAUNCHER
#!/bin/sh

cd /mnt/SDCARD/Apps/picoarch

LD_LIBRARY_PATH=./lib:$$LD_LIBRARY_PATH ./picoarch "$$@"
endef

dist-gmenu: $(foreach core, $(CORES), dist-gmenu-$(core)) dist-gmenu-picoarch
	cp README.trimui.md pkg/

# -- MinUI

ifeq ($(MINUI), 1)
define CORE_pak_template =

define $1_LAUNCH_SH
#!/bin/sh
# $($1_PAK_NAME).pak/launch.sh

EMU_EXE=picoarch
EMU_DIR=$$$$(dirname "$$$$0")
ROM_DIR=$$$${EMU_DIR/.pak/}
ROM_DIR=$$$${ROM_DIR/Emus/Roms}
EMU_NAME=$$$${ROM_DIR/\/mnt\/SDCARD\/Roms\//}
ROM=$$$${1}
$($1_PAK_EXTRA)
HOME="$$$$ROM_DIR"
cd "$$$$EMU_DIR"
LD_LIBRARY_PATH="$$$$EMU_DIR/lib:$$$$LD_LIBRARY_PATH" "$$$$EMU_DIR/$$$$EMU_EXE" ./$1_libretro.so "$$$$ROM" &> "/mnt/SDCARD/.minui/logs/$$$$EMU_NAME.txt"
endef

.PHONY: dist-minui-$(1)
dist-minui-$(1): $(BIN) $(1)_libretro.so
	mkdir -pv "pkg/MinUI/Emus/$($1_PAK_NAME).pak"
	$$(file >$1_launch.sh,$$($1_LAUNCH_SH))
	mv -v $1_launch.sh "pkg/MinUI/Emus/$($1_PAK_NAME).pak/launch.sh"
	cp -v $(BIN) $1_libretro.so "pkg/MinUI/Emus/$($1_PAK_NAME).pak"
	$(call install_licenses,pkg/MinUI/Emus/$($1_PAK_NAME).pak,$1)
	$(call install_liblz4,pkg/MinUI/Emus/$($1_PAK_NAME).pak)

endef

define picoarch_LAUNCH_SH
#!/bin/sh
# picoarch.pak/launch.sh

EMU_EXE=picoarch
EMU_DIR=$$(dirname "$$0")
EMU_NAME=$$EMU_EXE

needs-swap

HOME="/mnt/SDCARD/Games/picoarch.pak/"
cd "$$EMU_DIR"
LD_LIBRARY_PATH="$$EMU_DIR/lib:$$LD_LIBRARY_PATH" "$$EMU_DIR/$$EMU_EXE" &> "/mnt/SDCARD/.minui/logs/$$EMU_NAME.txt"
endef

dist-minui-picoarch: $(BIN) cores
	mkdir -pv pkg/MinUI/Games/picoarch.pak
	$(file >picoarch_launch.sh,$(picoarch_LAUNCH_SH))
	mv -v picoarch_launch.sh pkg/MinUI/Games/picoarch.pak/launch.sh
	cp -v $(BIN) $(SOFILES) pkg/MinUI/Games/picoarch.pak
	$(call install_licenses,pkg/MinUI/Games/picoarch.pak)
	$(call install_liblz4,pkg/MinUI/Games/picoarch.pak)
	find pkg/MinUI/Emus -name "*_libretro.txt" -exec cp {} pkg/MinUI/Games/picoarch.pak/LICENSES/ \;

$(foreach core, $(CORES),$(eval $(call CORE_pak_template,$(core))))

dist-minui: $(foreach core, $(CORES), dist-minui-$(core)) dist-minui-picoarch
	cp README.trimui.md pkg/

endif # MINUI=1

picoarch.zip:
	$(MAKE) platform=trimui PROFILE=APPLY clean-all dist-gmenu
	rm -fv $(OBJS) $(BIN)
	$(MAKE) platform=trimui PROFILE=APPLY EXTRA_CFLAGS=-Wno-error=coverage-mismatch MINUI=1 dist-minui
	cd pkg && zip -r ../picoarch.zip *
