.case		on

.include        "atari.inc"
.include 		"opcodes.inc"
.macpack        atari 

PLAYER1_DATA = $3C1F
BOTTOM_LINE = $6d ; $6e

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
.export     _array_ptr ; shared location with _array_index and _array_value
.export 	_lookup_index
.export 	_map_index
.export 	_video_ptr1
.export 	_video_ptr2
.export 	_text_ptr
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

;; these are exported for debugging
.export first_font_handler
.export second_font_handler
.export text_line_handler

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
_array_ptr:
_array_index: .byte 0
_array_value: .byte 0
_lookup_index: .byte 0
_map_index: .byte 0
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

ingame_font:
	.incbin "gfx_atari/ingame.fnt", 0 , 8 * 64

_display_list1:
    .byte   DL_BLK8
    .byte   DL_BLK8 
    .byte   DL_BLK7 | DL_DLI ; _dl_handler
    .byte   DL_BLK1
    .byte   DL_BLK1 | DL_DLI
		
    .repeat 24, index
		.byte   DL_CHR40x8x4 | DL_LMS | DL_DLI    ; first_font_handler and second_font_handler
		.word   (screen_memory1 + (40*index))
    .endrepeat
    .byte   DL_BLK1 
    .byte   DL_BLK1 | DL_DLI
    .byte   DL_BLK1
	
		.byte   DL_CHR40x8x1 | DL_LMS 
		.word   (text_line_memory)	
	
    .byte   DL_BLK1
    .byte   DL_JVB 
    .word   _display_list1 

_display_list2:
    .byte   DL_BLK8
    .byte   DL_BLK8 
    .byte   DL_BLK7 | DL_DLI
    .byte   DL_BLK1
    .byte   DL_BLK1 | DL_DLI
		
    .repeat 24, index
		.byte   DL_CHR40x8x4 | DL_LMS | DL_DLI
		.word   (screen_memory2 + (40*index))
    .endrepeat
    .byte   DL_BLK1
    .byte   DL_BLK1 | DL_DLI
    .byte   DL_BLK1
	
		.byte   DL_CHR40x8x1 | DL_LMS 
		.word   (text_line_memory)
	
    .byte   DL_BLK1
    .byte   DL_JVB 
    .word   _display_list2 	
	

screen_memory1:   
    .repeat 24*40
    .byte 0
    .endrepeat

screen_memory2:   
    .repeat 24*40
    .byte 0
    .endrepeat
	
text_line_memory:	
    .repeat 40
    .byte 0
    .endrepeat

gfx_end:

current_size = gfx_end-game_font1
end_address = $2000 + current_size
filler_size = PLAYER1_DATA - end_address

.out .sprintf( "SIZE: %04x", current_size ) 
.out .sprintf( "end_address: %04x", end_address ) 
.out .sprintf( "filler_size: %04x", filler_size ) 

.res filler_size, 0

; first player data
p0_start:

.byte 0
.byte 0
.byte 0
.byte 0
.byte %00110011
.byte %00110011
.byte %01100110
.byte %01100110
.byte %11001100
.byte %11001100
.byte %01100110
.byte %01100110
.byte %00110011
.byte %00110011


p0_end:
.res $100 - (p0_end-p0_start), 0

; second player data

p1_start:

.byte 0
.byte 0
.byte 0
.byte 0
.byte %11001100
.byte %11001100
.byte %01100110
.byte %01100110
.byte %00110011
.byte %00110011
.byte %01100110
.byte %01100110
.byte %11001100
.byte %11001100

p1_end:
.res $100 - (p1_end-p1_start), 0

; third player data

p2_start:

.byte %11111111
.byte %11111110
.byte %11111100
.byte %11111100

.res 188, %11111000

.byte %11111100
.byte %11111100
.byte %11111110
.byte %11111111

p2_end:
.res $100 - (p2_end-p2_start), 0

; fourth player data

p3_start:

.byte %11111111
.byte %01111111
.byte %00111111
.byte %00111111

.res 188, %00011111

.byte %00111111
.byte %00111111
.byte %01111111
.byte %11111111

p3_end:
.res $E0 - (p3_end-p3_start), 0

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
    .word   screen_memory1
_video_ptr2:    
    .word   screen_memory2
_text_ptr:    
    .word   text_line_memory

_background_color:
   .byte 0
   
.segment	"CODE"

.macro  set_dli  new_dli
	 lda #.lobyte(new_dli)
	 sta VDSLST
	 lda #.hibyte(new_dli)
	 sta VDSLST+1
.endmacro

_dl_handler:
 sta reg_a 
 ; set color of the playfield (the top part is covered by OS shadow register - black color)
 lda _background_color
 sta WSYNC
 sta COLBK
 set_dli first_font_handler  
 lda reg_a
 rti 
  
first_font_handler:
 sta reg_a
 ; lda _display_font_page1 ; replaced by self modifying code for speed
  .byte OPC_LDA_imm
_display_font_page1:
	.byte   .hibyte(game_font1) 
 sta WSYNC
 sta CHBASE

 lda VCOUNT ;VCOUNT
 cmp #BOTTOM_LINE ; if bottom of screen
 bcs set_text_handler   
 
 set_dli second_font_handler 
 lda reg_a
 rti

set_text_handler:
 set_dli text_line_handler 
 lda reg_a
 rti
 
second_font_handler:
 sta reg_a	
  ;; Set second font
 ; lda _display_font_page2 ; replaced by self modifying code for speed
 .byte OPC_LDA_imm
_display_font_page2:
	.byte   .hibyte(game_font2)
 sta WSYNC
 sta CHBASE

 lda VCOUNT ;VCOUNT
 cmp #BOTTOM_LINE ; if bottom of screen
 bcs set_text_handler   
  
 set_dli first_font_handler 
 lda reg_a
 rti 
 

text_line_handler:
 sta reg_a
 ; set black color below playfield
 lda #$00
 sta WSYNC
  
 sta COLBK
 sta COLPF2
 
 lda #.hibyte(ingame_font)
 sta CHBASE
  
 lda #$FF
 sta COLPF1

; set the first handler for the new frame
 set_dli _dl_handler   
 lda reg_a
 rti 

.out .sprintf( "DL HANDLERS SIZE: %04x", * - _dl_handler) 
