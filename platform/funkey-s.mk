SOURCES += plat_funkey.c funkey/fk_menu.c funkey/fk_instant_play.c
CFLAGS += -DCONTENT_DIR='"/mnt"' -DFUNKEY_S
LDFLAGS += -fPIC
LDFLAGS += -lSDL_image -lSDL_ttf # For fk_menu
core_platform = unix-armv7-hardfloat-neon

bluemsx_NAME = blueMSX
bluemsx_ROM_DIR = /mnt/MSX
bluemsx_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/dingux-msx.png
bluemsx_ICON = dingux-msx

dosbox-pure_ROM_DIR = /mnt/DOS
dosbox-pure_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/dosbox.png
dosbox-pure_ICON = dosbox

fake-08_NAME = fake-08
fake-08_ROM_DIR = /mnt/PICO-8
fake-08_ICON_URL = https://raw.githubusercontent.com/jtothebell/fake-08/master/platform/vita/sce_sys/icon0.png
fake-08_ICON = icon0

fbalpha2012_NAME = fba2012
fbalpha2012_ROM_DIR = /mnt/Arcade
fbalpha2012_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/fba.png
fbalpha2012_ICON = fba

fceumm_ROM_DIR = /mnt/NES
fceumm_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/FCEUX/opk/nes/nes.png
fceumm_ICON = nes

fmsx_NAME = fMSX
fmsx_ROM_DIR = /mnt/MSX
fmsx_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/dingux-msx.png
fmsx_ICON = dingux-msx

gambatte_ROM_DIR = /mnt/Game Boy
gambatte_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/gnuboy/opk/gb/gb.png
gambatte_ICON = gb

gme_ROM_DIR = /mnt/Music
gme_TYPES = ay,gbs,gym,hes,kss,nsf,nsfe,sap,spc,vgm,vgz,zip
gme_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/gmu.png
gme_ICON = gmu

gpsp_ROM_DIR = /mnt/Game Boy Advance
gpsp_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/gpsp/opk/gba/gba.png
gpsp_ICON = gba

mame2000_ROM_DIR = /mnt/Arcade
mame2000_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/mame.png
mame2000_ICON = mame

mame2003_plus_NAME = mame2003+
mame2003_plus_ROM_DIR = /mnt/Arcade
mame2003_plus_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/mame.png
mame2003_plus_ICON = icon

mednafen_ngp_NAME = ngp
mednafen_ngp_ROM_DIR = /mnt/Neo Geo Pocket
mednafen_ngp_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/mednafen/opk/ngp/ngp.png
mednafen_ngp_ICON = ngp

mednafen_pce_fast_NAME = pce_fast
mednafen_pce_fast_ROM_DIR = /mnt/PCE-TurboGrafx
mednafen_pce_fast_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/mednafen/opk/pce/pce.png
mednafen_pce_fast_ICON = pce

mednafen_wswan_NAME = wswan
mednafen_wswan_ROM_DIR = /mnt/WonderSwan
mednafen_wswan_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/mednafen/opk/wonderswan/wonderswan.png
mednafen_wswan_ICON = wonderswan

pcsx_rearmed_ROM_DIR = /mnt/PS1
pcsx_rearmed_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/pcsx4all.png
pcsx_rearmed_ICON = pcsx4all

picodrive_ROM_DIR = /mnt/Sega Genesis
picodrive_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/picodrive/opk/megadrive/megadrive.png
picodrive_ICON = megadrive

pokemini_ROM_DIR = /mnt/PokeMini
pokemini_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/pokemini.png
pokemini_ICON = pokemini

quicknes_ROM_DIR = /mnt/NES
quicknes_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/FCEUX/opk/nes/nes.png
quicknes_ICON = nes

smsplus-gx_ROM_DIR = /mnt/Game Gear
smsplus-gx_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/mednafen/opk/gamegear/gamegear.png
smsplus-gx_ICON = gamegear

snes9x2002_ROM_DIR = /mnt/SNES
snes9x2002_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/PocketSNES/opk/snes/snes.png
snes9x2002_ICON = snes

snes9x2005_ROM_DIR = /mnt/SNES
snes9x2005_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/PocketSNES/opk/snes/snes.png
snes9x2005_ICON = snes

snes9x2005_plus_NAME = snes9x2005+
snes9x2005_plus_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/PocketSNES/opk/snes/snes.png
snes9x2005_plus_ICON = snes
snes9x2005_plus_ROM_DIR = /mnt/SNES

snes9x2010_NAME = snes9x2010
snes9x2010_ICON_URL = https://raw.githubusercontent.com/FunKey-Project/FunKey-OS/master/FunKey/package/PocketSNES/opk/snes/snes.png
snes9x2010_ICON = snes
snes9x2010_ROM_DIR = /mnt/SNES

stella2014_NAME = stella2014
stella2014_ICON_URL = https://raw.githubusercontent.com/MiyooCFW/gmenu2x/gmenunx/assets/miyoo/skins/PixUI/icons/stella-od.png
stella2014_ICON = stella-od
stella2014_ROM_DIR = /mnt/Atari 2600

define CORE_opk =

$1_NAME ?= $1

define $1_DESKTOP
[Desktop Entry]
Name=$$($1_NAME)
Comment=
Exec=env LD_LIBRARY_PATH=./lib:$$$$LD_LIBRARY_PATH ./picoarch ./$1_libretro.so %f
Icon=$$($1_ICON)
SelectorBrowser=true
SelectorDir=$($1_ROM_DIR)
SelectorFilter=$($1_TYPES)
Terminal=false
Type=Application
StartupNotify=true
Categories=emulators;
endef

picoarch-$(1).opk: $(BIN) $(1)_libretro.so
	mkdir -pv .opkdata
	$(call install_licenses,.opkdata,$1)
	$(call install_liblz4,.opkdata)
	$$(file >$$($(1)_NAME).funkey-s.desktop,$$($(1)_DESKTOP))
	mv -v $$($(1)_NAME).funkey-s.desktop .opkdata
	cp -v $(BIN) $(1)_libretro.so .opkdata
	$(if $($(1)_ICON_URL),cd .opkdata && curl -L $($(1)_ICON_URL) -O && mogrify -resize '32x32>' $($(1)_ICON).png,)
	cd .opkdata && mksquashfs * ../$$@ -all-root -no-xattrs -noappend -no-exports
	rm -dRv .opkdata
endef

$(foreach core, $(CORES),$(eval $(call CORE_opk,$(core))))

define picoarch_DESKTOP
[Desktop Entry]
Name=picoarch
Comment=Small screen libretro frontend
Exec=env LD_LIBRARY_PATH=./lib:$$LD_LIBRARY_PATH ./picoarch
Icon=sdlretro_icon
Terminal=false
Type=Application
StartupNotify=true
Categories=emulators;
endef

picoarch.opk: $(BIN) $(SOFILES)
	mkdir -p .opkdata
	$(file >picoarch.funkey-s.desktop,$(picoarch_DESKTOP))
	mv picoarch.funkey-s.desktop .opkdata
	cp $(BIN) $(SOFILES) .opkdata
	$(call install_licenses,.opkdata,$1)
	$(call install_liblz4,.opkdata)
	cd .opkdata && curl -L -O https://raw.githubusercontent.com/FunKey-Project/sdlretro/master/data/sdlretro_icon.png
	cd .opkdata && mksquashfs * ../$@ -all-root -no-xattrs -noappend -no-exports
	rm -r .opkdata

define picoarch_lite_DESKTOP
[Desktop Entry]
Name=picoarch-lite
Comment=Small screen libretro frontend
Exec=env LD_LIBRARY_PATH=./lib:$$LD_LIBRARY_PATH ./picoarch %f
Icon=sdlretro_icon
SelectorBrowser=true
SelectorDir=/mnt/Libretro/cores
SelectorFilter=so
Terminal=false
Type=Application
StartupNotify=true
Categories=emulators;
endef

picoarch-lite.opk: $(BIN)
	mkdir -p .opkdata
	$(file >picoarch.funkey-s.desktop,$(picoarch_lite_DESKTOP))
	mv picoarch.funkey-s.desktop .opkdata
	cp $(BIN) .opkdata
	$(call install_licenses,.opkdata,$1)
	$(call install_liblz4,.opkdata)
	cd .opkdata && curl -L -O https://raw.githubusercontent.com/FunKey-Project/sdlretro/master/data/sdlretro_icon.png
	cd .opkdata && mksquashfs * ../$@ -all-root -no-xattrs -noappend -no-exports
	rm -r .opkdata

picoarch-funkey-s.zip: picoarch.opk $(foreach core, $(CORES), picoarch-$(core).opk)
	rm -f $@
	zip $@ $^

cores-funkey-s.zip: $(SOFILES)
	rm -f $@
	zip $@ $^

.PHONY: dist
dist: picoarch-lite.opk picoarch-funkey-s.zip cores-funkey-s.zip
