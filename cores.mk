bluemsx_REPO = https://github.com/libretro/blueMSX-libretro
bluemsx_LICENSE = license.txt
bluemsx_TYPES = rom,ri,mx1,mx2,dsk,col,sg,sc,cas,m3u

chimerasnes_REPO = https://github.com/jamsilva/chimerasnes
chimerasnes_LICENSE = LICENSES
chimerasnes_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,bs,swc,st

dosbox-pure_REPO = https://github.com/schellingb/dosbox-pure
dosbox-pure_LICENSE = LICENSE
dosbox-pure_CORE = dosbox_pure_libretro.so
dosbox-pure_TYPES = zip,dosz,exe,com,bat,iso,cue,ins,img,ima,vhd,jrc,tc,m3u,m3u8,conf
dosbox-pure_FLAGS = STRIPCMD="$(CROSS_COMPILE)strip"
ifeq ($(platform), funkey-s)
dosbox-pure_FLAGS += CYCLE_LIMIT=8200
endif

ecwolf_REPO = https://github.com/libretro/ecwolf
ecwolf_LICENSE = CONTRIBUTING.md
ecwolf_BUILD_PATH = ecwolf/src/libretro
ecwolf_TYPES = wl6,n3d,sod,sdm,wl1,pk3

fake-08_REPO = https://github.com/jtothebell/fake-08
fake-08_LICENSE = LICENSE.MD
fake-08_BUILD_PATH = fake-08/platform/libretro
fake-08_MAKEFILE = Makefile
fake-08_CORE = fake08_libretro.so
fake-08_TYPES = p8,png

fbalpha2012_BUILD_PATH = fbalpha2012/svn-current/trunk
fbalpha2012_LICENSE = svn-current/trunk/src/license.txt
fbalpha2012_MAKEFILE = makefile.libretro
fbalpha2012_TYPES = zip

fceumm_REPO = https://github.com/libretro/libretro-fceumm
fceumm_LICENSE = Copying
fceumm_MAKEFILE = Makefile.libretro
fceumm_TYPES = fds,nes,unf,unif

fmsx_REPO = https://github.com/libretro/fmsx-libretro
fmsx_LICENSE = LICENSE
fmsx_TYPES = rom,mx1,mx2,dsk,cas

gambatte_REPO = https://github.com/libretro/gambatte-libretro
gambatte_LICENSE = COPYING
gambatte_TYPES = gb,gbc,dmg,zip

gme_REPO = https://github.com/libretro/libretro-gme
gme_LICENSE = LICENSE

gpsp_LICENSE = COPYING
gpsp_TYPES = gba,bin,zip

mame2000_REPO = https://github.com/libretro/mame2000-libretro
mame2000_LICENSE = readme.txt
mame2000_TYPES = zip

mame2003_plus_REPO = https://github.com/libretro/mame2003-plus-libretro
mame2003_plus_LICENSE = LICENSE.md
mame2003_plus_TYPES = zip

mednafen_lynx_REPO = https://github.com/libretro/beetle-lynx-libretro
mednafen_lynx_LICENSE = COPYING
mednafen_lynx_TYPES = lnx,lyx,bll,o

mednafen_ngp_REPO = https://github.com/libretro/beetle-ngp-libretro
mednafen_ngp_LICENSE = COPYING
mednafen_ngp_TYPES = ngp,ngc,ngpc,npc

mednafen_pce_fast_REPO = https://github.com/libretro/beetle-pce-fast-libretro
mednafen_pce_fast_LICENSE = COPYING
mednafen_pce_fast_TYPES = pce,cue,ccd,chd,toc,m3u

mednafen_wswan_REPO = https://github.com/libretro/beetle-wswan-libretro
mednafen_wswan_LICENSE = COPYING
mednafen_wswan_TYPES = ws,wsc,pc2

pcsx_rearmed_LICENSE = COPYING
pcsx_rearmed_MAKEFILE = Makefile.libretro
pcsx_rearmed_TYPES = bin,cue,img,mdf,pbp,toc,cbn,m3u,chd

picodrive_LICENSE = COPYING
picodrive_MAKEFILE = Makefile.libretro
picodrive_TYPES = bin,gen,smd,md,32x,cue,iso,chd,sms,gg,m3u,68k,sgd

pokemini_LICENSE = LICENSE
pokemini_TYPES = min

prboom_REPO = https://github.com/DrUm78/libretro-prboom
prboom_LICENSE = COPYING
prboom_TYPES = wad,iwad,pwad,lmp

quicknes_REPO = https://github.com/libretro/QuickNES_Core
quicknes_LICENSE = LICENSE
quicknes_TYPES = nes

scummvm_LICENSE = COPYING
scummvm_TYPES = scummvm

smsplus-gx_LICENSE = docs/license
smsplus-gx_MAKEFILE = Makefile.libretro
smsplus-gx_CORE = smsplus_libretro.so
smsplus-gx_TYPES = sms,bin,rom,col,gg,sg

snes9x2002_LICENSE = libretro/libretro.c
snes9x2002_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,swc,zip

snes9x2005_REPO = https://github.com/libretro/snes9x2005
snes9x2005_LICENSE = copyright
snes9x2005_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,swc,zip

snes9x2005_plus_REPO = https://github.com/libretro/snes9x2005
snes9x2005_plus_LICENSE = copyright
snes9x2005_plus_FLAGS = USE_BLARGG_APU=1
snes9x2005_plus_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,swc,zip

snes9x2010_LICENSE = LICENSE.txt
snes9x2010_TYPES = smc,fig,sfc,gd3,gd7,dx2,bsx,swc,zip

stella2014_REPO = https://github.com/libretro/stella2014-libretro
stella2014_LICENSE = stella/license.txt
stella2014_TYPES = a26,bin

tyrquake_REPO = https://github.com/DrUm78/tyrquake
tyrquake_LICENSE = LICENSE.txt
tyrquake_TYPES = pak

vitaquake2_REPO = https://github.com/DrUm78/vitaquake2
vitaquake2_LICENSE = LICENSE
vitaquake2_TYPES = pak
