;
;  hires.s
;  cc65 Chess
;
;  Created by Oliver Schmidt, January 2020.
;
;

.export _hires_CharSet, _hires_Pieces
.export _hires_Init, _hires_Done, _hires_Text, _hires_Draw,  _hires_Mask

.include "ebadger.inc"
.include "zeropage.inc"

.import popa, popax



.rodata


BASELO:
    .repeat $C0, I
    .byte   I & $08 << 4 | I & $C0 >> 1 | I & $C0 >> 3
    .endrep

BASEHI:
    .repeat $C0, I
    .byte   >$2000 | I & $07 << 2 | I & $30 >> 4
    .endrep

_hires_CharSet:
.incbin "charset.bin"

_hires_Pieces:
.incbin "pieces.bin"

.code


.proc   _hires_Init    
        lda     #$80
        sta     $C043

        lda     #$00
        sta     WNDTOP      ; Prepare hires_text()
        rts

.endproc


.proc   _hires_Done
        lda     #$00
        sta     $C043
        sta     WNDTOP      ; Back to full screen text
        rts

.endproc


.proc   _hires_Text
        sta $C043
        rts

.endproc


.data


.proc   _hires_Draw

        sta     src+1       ; 'src' lo
        dec     src+1
        
        stx     src+2       ; 'src' hi

        jsr     popax       ; 'rop'
        stx     rop
        sta     rop+1

        jsr     popa        ; 'ysize'
        sta     ymax+1

        jsr     popa        ; 'xsize'
        sta     xmax+1

        jsr     popa        ; 'ypos'
        sta     ypos+1
        tax

        clc
        adc     ymax+1
        sta     ymax+1

        jsr     popa        ; 'xpos'
        sta     xpos+1

        clc
        adc     xmax+1
        sta     xmax+1
yloop:
        lda     BASELO,x
        sta     dst+1
        lda     BASEHI,x
        sta     dst+2

xpos:   ldx     #$FF        ; Patched
xloop:
src:    lda     $FFFF,y     ; Patched
        iny
rop:    nop                 ; Patched
        nop                 ; Patched
dst:    sta     $FFFF,x     ; Patched
        inx
xmax:   cpx     #$FF        ; Patched
        bne     xloop

        inc     ypos+1
ypos:   ldx     #$FF        ; Patched
ymax:   cpx     #$FF        ; Patched
        bne     yloop
        rts

.endproc


.proc   _hires_Mask

        stx     rop         ; 'rop' hi
        sta     rop+1       ; 'rop' lo

        jsr     popa        ; 'ysize'
        sta     ymax+1

        jsr     popa        ; 'xsize'
        sta     xmax+1

        jsr     popa        ; 'ypos'
        tax

        clc
        adc     ymax+1
        sta     ymax+1

        jsr     popa        ; 'xpos'
        sta     xpos+1

        clc
        adc     xmax+1
        sta     xmax+1

yloop:
        lda     BASELO,x
        sta     src+1
        sta     dst+1
        lda     BASEHI,x
        sta     src+2
        sta     dst+2

xpos:   ldy     #$FF        ; Patched
xloop:
src:    lda     $FFFF,y     ; Patched
rop:    nop                 ; Patched
        nop                 ; Patched
dst:    sta     $FFFF,y     ; Patched
        iny
xmax:   cpy     #$FF        ; Patched
        bne     xloop

        inx
ymax:   cpx     #$FF        ; Patched
        bne     yloop
        rts

.endproc
