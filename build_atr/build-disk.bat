del riu.atr
copy /Y ..\gfx_atari\0.fnt .
copy /Y ..\gfx_atari\1.fnt .
copy /Y ..\gfx_atari\2.fnt .
copy /Y ..\gfx_atari\3.fnt .
copy /Y ..\gfx_atari\4.fnt .
copy /Y ..\gfx_atari\5.fnt .
copy /Y ..\gfx_atari\6.fnt .
copy /Y ..\gfx_atari\7.fnt .
copy /Y ..\gfx_atari\8.fnt .
copy /Y ..\gfx_atari\0.inv .
copy /Y ..\gfx_atari\1.inv .
copy /Y ..\gfx_atari\2.inv .
copy /Y ..\gfx_atari\3.inv .
copy /Y ..\gfx_atari\4.inv .
copy /Y ..\gfx_atari\5.inv .
copy /Y ..\gfx_atari\6.inv .
copy /Y ..\gfx_atari\7.inv .
copy /Y ..\gfx_atari\8.inv .
mkatr riu.atr -b XBW130.DOS riu.xex LEVELS.RIU LEVELS.ATL STARTUP.BAT 0.fnt 1.fnt 2.fnt 3.fnt 4.fnt 5.fnt 6.fnt 7.fnt 8.fnt 0.inv 1.inv 2.inv 3.inv 4.inv 5.inv 6.inv 7.inv 8.inv
copy riu.atr ..
cd ..
start riu.atr
