.case		on                                                                                                    
;.export		_rom_off, _rom_on, _rom_copy
.export		_rom_copy
.segment	"BANKCODE" 

; This could be done in "pre-load" segment of executable 
; Taken from AtariWiki, by Russ Gilbert

prev_022F: .byte 0
prev_D40E: .byte 0
prev_D20E: .byte 0
zp_ptr = $F0

_rom_copy: 
  LDA $022F
  STA prev_022F
  LDA $D40E
  STA prev_D40E
  LDA $D20E
  STA prev_D20E

  LDA #$0
  STA $D400   ;TURN OFF ANTIC
  STA $D40E
  STA $D20E   ;DISABLE INTRPTS
  SEI         ;MAKE SURE NO
  LDA #$C0    ;INTERRUPTS
  STA zp_ptr+1     ;SET UP BANK AREA
  LDY #0
  STY zp_ptr
  
  ;jmp cleanup

L3:
  LDA (zp_ptr),Y ;GET ROM BYTE
  TAX 
  LDA #$FE
  AND $D301   ;CLR BIT 0 TO
  STA $D301   ;BANK TO OS RAM
  TXA 
  STA (zp_ptr),Y ;STORE BYTE
  LDA #$01
  ORA $D301
  STA $D301   ;BACK TO ROM
  INY         ;GET READY NEXT
  CPY #0
  BNE L3      ;256 TIMES

NOK:
  INC zp_ptr+1
  CLC          ;DON'T UNDRSTND
  LDA zp_ptr+1 ;THIS CNTRL LOOP
  CMP #$D0
  BCC T1      ;FROM $C0-$FF
  CMP #$D8    ;SKIP $D0-$D7
  BCC NOK

T1:
  CMP #$00
  BNE L3
  CLC 

  LDA #$01
  ORA $D301
  STA $D301   ;ROM is active
  
cleanup:

  LDA prev_022F
  STA $022F
  LDA prev_D40E
  STA $D40E
  LDA prev_D20E  
  STA $D20E

  CLI 
  RTS 
  
;_rom_on:
;  LDA #$1
;  ORA $D301    ;SET BIT 0 TO 1
;  STA $D301   
;  rts

;_rom_off:
;  LDA #$FE
;  AND $D301   ;CLR BIT 0 TO
;  STA $D301   ;BANK TO OS RAM
;  rts

;start:
;  jsr _rom_off
;  jmp (DOSVEC)
;  INI start