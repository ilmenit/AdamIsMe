@del sfx.info
@del sfx.xex
mads.exe sfx.a65 -hc:sfx.h -o:sfx.xex -l:sfx.lst -t:sfx.lbl
chkxex sfx.xex > sfx.info
type sfx.info
