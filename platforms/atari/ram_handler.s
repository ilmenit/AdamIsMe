;
; File generated by cc65 v 2.18 - Git f75657d
;
	.fopt		compiler,"cc65 v 2.18 - Git f75657d"
	.setcpu		"6502"
	.smart		on
	.autoimport	on
	.case		on
	.debuginfo	off
	.importzp	sp, sreg, regsave, regbank
	.importzp	tmp1, tmp2, tmp3, tmp4, ptr1, ptr2, ptr3, ptr4
	.macpack	longbranch
	.export		_memory_handler_init
	.export		_memory_objects_in_bank
	.export		_memory_objects_in_all_banks
	.export		_get_banked_address
	.export		_memory_select_bank
	.export		_bank_test
	.export		_bank_selection
	.export		_value_in_bank
	.export		_prev_portb
	.export		_memory_banks
	.export		_bank_ptr
	.export		_bank_size

.segment	"BANKDATA"

.segment	"BANKDATA"
_bank_test:
	.byte	$00
.segment	"BANKDATA"
.segment	"DATA"
_bank_selection:
	.byte	$00
	.byte	$04
	.byte	$08
	.byte	$0C
_value_in_bank:
	.byte	$00
_prev_portb:
	.byte	$00
_memory_banks:
	.byte	$00
_bank_ptr:
	.word	$D800
_bank_size:
	.word	$0C00

.segment	"BANKRODATA"

.segment	"BANKRODATA"
.segment	"BANKRODATA"
.segment	"RODATA"

; ---------------------------------------------------------------
; void __near__ memory_handler_init (void)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_memory_handler_init: near

.segment	"CODE"

;
; prev_portb = PIA.portb;
;
	lda     $D301
	sta     _prev_portb
;
; memory_select_bank(3);
;
	lda     #$03
	jsr     _memory_select_bank
;
; value_in_bank = bank_test;
;
	lda     _bank_test
	sta     _value_in_bank
;
; memory_select_bank(BANK_NONE);
;
	lda     #$FF
	jsr     _memory_select_bank
;
; ++bank_test;
;
	inc     _bank_test
;
; memory_select_bank(3); 
;
	lda     #$03
	jsr     _memory_select_bank
;
; if (bank_test == value_in_bank)
;
	lda     _value_in_bank
	cmp     _bank_test
	bne     L0088
;
; memory_banks = 4;
;
	lda     #$04
	sta     _memory_banks
;
; bank_size = BANK_SIZE;
;
	ldx     #$40
	lda     #$00
	sta     _bank_size
	stx     _bank_size+1
;
; bank_ptr = BANK_ADDRESS;
;
	sta     _bank_ptr
	stx     _bank_ptr+1
;
; else
;
	jmp     L006B
;
; memory_banks = 1;
;
L0088:	lda     #$01
	sta     _memory_banks
;
; memory_select_bank(BANK_NONE); // restore the previous bank, because we succesfuly changed it
;
L006B:	lda     #$FF
	jmp     _memory_select_bank

.endproc

; ---------------------------------------------------------------
; unsigned int __near__ memory_objects_in_bank (unsigned int)
; ---------------------------------------------------------------

.segment	"BANKCODE"

.proc	_memory_objects_in_bank: near

.segment	"BANKCODE"

;
; {
;
	jsr     pushax
;
; return bank_size / object_size;
;
	lda     _bank_size
	ldx     _bank_size+1
	jsr     pushax
	ldy     #$03
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	jsr     tosudivax
;
; }
;
	jmp     incsp2

.endproc

; ---------------------------------------------------------------
; unsigned int __near__ memory_objects_in_all_banks (unsigned int)
; ---------------------------------------------------------------

.segment	"BANKCODE"

.proc	_memory_objects_in_all_banks: near

.segment	"BANKCODE"

;
; {
;
	jsr     pushax
;
; return memory_objects_in_bank(object_size) * (unsigned int)memory_banks;
;
	ldy     #$01
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	jsr     _memory_objects_in_bank
	jsr     pushax
	lda     _memory_banks
	jsr     tosumula0
;
; }
;
	jmp     incsp2

.endproc

; ---------------------------------------------------------------
; __near__ unsigned char * __near__ get_banked_address (unsigned int, unsigned int)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_get_banked_address: near

.segment	"BSS"

L0071:
	.res	2,$00
L0072:
	.res	1,$00

.segment	"CODE"

;
; {
;
	jsr     pushax
;
; objects_in_bank = memory_objects_in_bank(object_size);
;
	ldy     #$01
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	jsr     _memory_objects_in_bank
	sta     L0071
	stx     L0071+1
;
; bank_number = (unsigned char) (object_index / objects_in_bank);
;
	ldy     #$05
	jsr     pushwysp
	lda     L0071
	ldx     L0071+1
	jsr     tosudivax
	sta     L0072
;
; if (bank_number >= memory_banks)
;
	ldx     #$00
	lda     L0072
	cmp     _memory_banks
	bcc     L0079
;
; return NULL;
;
	txa
	jmp     incsp4
;
; memory_select_bank(bank_number);
;
L0079:	lda     L0072
	jsr     _memory_select_bank
;
; return &bank_ptr[(object_index % objects_in_bank) * object_size];
;
	lda     _bank_ptr
	ldx     _bank_ptr+1
	jsr     pushax
	ldy     #$07
	jsr     pushwysp
	lda     L0071
	ldx     L0071+1
	jsr     tosumodax
	jsr     pushax
	ldy     #$05
	lda     (sp),y
	tax
	dey
	lda     (sp),y
	jsr     tosumulax
	jsr     tosaddax
;
; }
;
	jmp     incsp4

.endproc

; ---------------------------------------------------------------
; void __near__ memory_select_bank (unsigned char)
; ---------------------------------------------------------------

.segment	"CODE"

.proc	_memory_select_bank: near

.segment	"CODE"

;
; {
;
	jsr     pusha
;
; if (bank == BANK_NONE)
;
	ldy     #$00
	lda     (sp),y
	cmp     #$FF
	bne     L008A
;
; PIA.portb = prev_portb;
;
	lda     _prev_portb
;
; else
;
	jmp     L0089
;
; if (memory_banks == 1)
;
L008A:	lda     _memory_banks
	cmp     #$01
	bne     L008B
;
; PIA.portb = (prev_portb & 0b11111110);
;
	lda     _prev_portb
	and     #$FE
;
; else
;
	jmp     L0089
;
; PIA.portb = (prev_portb & 0b11000011) | bank_selection[bank];
;
L008B:	lda     _prev_portb
	and     #$C3
	jsr     pusha0
	ldy     #$02
	lda     (sp),y
	tay
	lda     _bank_selection,y
	jsr     tosora0
L0089:	sta     $D301
;
; }
;
	jmp     incsp1

.endproc

