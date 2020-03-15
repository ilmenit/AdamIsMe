rem @echo off
setlocal EnableDelayedExpansion
cd d:\Projekty\RobboIsYou
SET PATH=%PATH%;C:\CC65\BIN
SET CC65_HOME=C:\CC65
SET CC65_INC=C:\CC65\INCLUDE
SET LD65_LIB=C:\CC65\LIB
SET LD65_CFG=C:\CC65\CFG 
del riu.xex

set all_files=

for %%x in (
	platforms/atari/undo_redo
	platforms/atari/ram_handler
	platforms/atari/atari_fn
	platforms/atari/sounds
	common/level
	common/lookup
	common/you
	common/interact
	common/data
	common/move
	common/rules
	common/main 
) do (
	cc65 -Cl -T -Osir --cpu 6502 -t atari --code-name "BANKCODE" --data-name "BANKDATA" --rodata-name "BANKRODATA" --bss-name "BANKDATA" %%x.c || goto :error
	ca65 -t atari %%x.s || goto :error
	set all_files=!all_files! %%x.o
)

for %%x in (
	platforms/atari/code
	platforms/atari/linker
	platforms/atari/rom_off
) do (
	ca65 -t atari %%x.asm || goto :error
	set all_files=!all_files! %%x.o
)

ld65 -Ln game.lbl --mapfile game.map --dbgfile riu.dbg -C atari-atr.cfg -o riu.xex %all_files% atari.lib

@IF NOT EXIST riu.xex (
	@echo Compilation error
	goto :error
)

@if not errorlevel 0 goto error
copy /Y riu.xex build_atr
copy /Y level_set\levels.riu build_atr
copy /Y level_set\levels.atl build_atr
cd build_atr
build-disk.bat
goto end
:error
	pause
:end
exit /b 0