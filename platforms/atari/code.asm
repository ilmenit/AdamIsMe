.case		on

.include        "atari.inc"
.include 		"opcodes.inc"
.macpack        atari 

PLAYER1_DATA = $3C20

.export 	_local_x
.export 	_local_y
.export 	_local_index
.export 	_local_type
.export 	_local_text_type
.export 	_local_flags
.export 	_local_temp1
.export 	_local_temp2
.export 	_array_index
.export 	_array_value
.export 	_map_index
.export 	_video_ptr1
.export 	_video_ptr2
.export 	_display_list1
.export 	_display_list2
.export     _display_font_page1
.export     _display_font_page2
.export     _game_font_page1
.export     _game_font_page2
.export		_game_font_address
.export		_galaxy_font_page1
.export		_galaxy_font_page2
.export		_galaxy_font_address
.export		_dl_handler
.export		_background_color

.segment "ZEROPAGE"
regA: .byte 0
_local_x: .byte 0
_local_y: .byte 0
_local_index: .byte 0
_local_type: .byte 0
_local_text_type: .byte 0
_local_flags: .byte 0
_local_temp1: .byte 0
_local_temp2: .byte 0
_map_index: .byte 0
_array_index: .byte 0
_array_value: .byte 0
reg_a: .byte 0

.segment "SFX"
; it has EXE header, so we skip first 2 bytes
.incbin "sfx/sfx.xex",2

.segment "GFX"

game_font1:
.repeat 4, i
	.incbin "gfx_atari/1.fnt",i*2*32*8,32*8
.endrepeat

game_font2:
.repeat 4, i
	.incbin "gfx_atari/1.fnt",i*2*32*8+32*8,32*8
.endrepeat

_galaxy_font1:
.repeat 4, i
	.incbin "gfx_atari/0.fnt",i*2*32*8,32*8
.endrepeat

_galaxy_font2:
.repeat 4, i
	.incbin "gfx_atari/0.fnt",i*2*32*8+32*8,32*8
.endrepeat

_display_list1:
    .byte   DL_BLK8
    .byte   DL_BLK8 
    .byte   DL_BLK8 | DL_DLI
    .repeat 24, index
		.byte   DL_CHR40x8x4 | DL_LMS | DL_DLI
		.word   (_screen_memory1 + (40*index))
    .endrepeat
    .byte   DL_BLK1 | DL_DLI 
    .byte   DL_JVB 
    .word   _display_list1 

_display_list2:
    .byte   DL_BLK8
    .byte   DL_BLK8 
    .byte   DL_BLK8 | DL_DLI
    .repeat 24, index
		.byte   DL_CHR40x8x4 | DL_LMS | DL_DLI
		.word   (_screen_memory2 + (40*index))
    .endrepeat
    .byte   DL_BLK1 | DL_DLI 
    .byte   DL_JVB 
    .word   _display_list2 	
	

_screen_memory1:   
    .repeat 24*40
    .byte 0
    .endrepeat

_screen_memory2:   
    .repeat 24*40
    .byte 0
    .endrepeat

gfx_end:

current_size = gfx_end-game_font1
end_address = $2000 + current_size
filler_size = PLAYER1_DATA - end_address

;.out .sprintf( "SIZE: %04x", current_size ) 
;.out .sprintf( "end_address: %04x", end_address ) 
;.out .sprintf( "filler_size: %04x", filler_size ) 

.res filler_size, 0

p1_start:

; first player data

.byte %11100000
.byte %11000000
.byte %10000000
.byte %10000000

.res 184, 0

.byte %10000000
.byte %10000000
.byte %11000000
.byte %11100000

p1_end:

.res $100 - (p1_end-p1_start), 0

p2_start:

; second player data

.byte %00000111
.byte %00000011
.byte %00000001
.byte %00000001

.res 184, 0

.byte %00000001
.byte %00000001
.byte %00000011
.byte %00000111


.segment "DATA"

;;;;;;;;;;;;;;; These point to specific pages 

_game_font_address:
	.byte   0
_game_font_page1:
	.byte   .hibyte(game_font1)

_game_font_page2:
	.byte   .hibyte(game_font2)

_galaxy_font_address:
	.byte   0
_galaxy_font_page1:
	.byte   .hibyte(_galaxy_font1)

_galaxy_font_page2:
	.byte   .hibyte(_galaxy_font2)

_video_ptr1:    
    .word   _screen_memory1
_video_ptr2:    
    .word   _screen_memory2

_background_color:
   .byte 0

.segment	"CODE"

_dl_handler:
 sta reg_a
 lda $D40B ;VCOUNT
 sta WSYNC
 cmp #$6e ; if bottom of screen
 bcs bottom_handler
 cmp #$10 ; if bottom of screen
 bcc up_handler
 ; every 8th line change the font set
 and #%00000100
 beq set_second
 ;; Set first font
 ;lda _display_font_page1 ; replaced by self modifying code for speed
  .byte OPC_LDA_imm
_display_font_page1:
	.byte   .hibyte(game_font1) 
 sta CHBASE
 lda reg_a
 rti

set_second:
  ;; Set second font
 ; lda _display_font_page2 ; replaced by self modifying code for speed
 .byte OPC_LDA_imm
_display_font_page2:
	.byte   .hibyte(game_font2)
 sta CHBASE
 lda reg_a
 rti 
 
bottom_handler:
 ; set black color below playfield
 lda #$00
 sta $D01A
 lda reg_a
 rti 

up_handler:
 ; set color of the playfield (the top part is covered by OS shadow register - black color)
 lda _background_color
 sta $D01A
 lda reg_a
 rti 
