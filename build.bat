rem @echo off
cd d:\Projekty\RobboIsYou
SET PATH=%PATH%;C:\CC65\BIN
SET CC65_HOME=C:\CC65
SET CC65_INC=C:\CC65\INCLUDE
SET LD65_LIB=C:\CC65\LIB
SET LD65_CFG=C:\CC65\CFG 
del riu.xex
cl65 -C atari-atr.cfg -t atari -T -Cl -Osir -o main.xex platforms/atari/code.asm platforms/atari/atari_fn.c platforms/atari/sounds.c common/level.c common/lookup.c common/you.c common/interact.c common/data.c common/move.c common/rules.c  common/main.c || goto :error
@if not errorlevel 0 goto error
copy /Y level_set\levels.riu build_atr
merge-xex platforms\atari\sfx\sfx.xex main.xex riu.xex
cd build_atr
build-disk.bat
goto end
:error
:end
exit /b 0