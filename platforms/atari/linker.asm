    .import __GFX_LOAD__, __FIRST_END_LOAD__
    .import __LOWCODE_LOAD__, __BSS_LOAD__
	.import __MAIN_START__
	
	.export         __FILEHDR__: absolute = 1
    .import         __MAIN_START__, __BSS_LOAD__
	
.segment "FILEHDR"
     .word   $FFFF

.segment "NEXEHDR"
    .word    __GFX_LOAD__
    .word    __FIRST_END_LOAD__ - 1

.segment "CHKHDR"
    .word    __LOWCODE_LOAD__
    .word    __BSS_LOAD__ - 1
	

.segment "FIRST_END"
