;   lighXTforms - 8088/CGA - 256 bytes - demodulation 2o21
;   -- wbcbz7 z7.lo.zozl
;
;   partysmashed lol
;   greets to everyone and everybody something something fuck fuck aaaaaa
;
;   lxtf.com 	 - cga     version             (tested on original IBM PC 5155)
;   lxtf_ega.com - ega\vga compatible version  
;
;   sound pitch depends on cpu\vram speed, no time to fix this
;
;   also check my MS-DOS demoscene seminar @ https://github.com/wbcbz7/yadm21_seminar
;

        org     0x100
        use16
        
        
; data start address
rdata       equ     0x800

width       equ     40
height      equ     25


sprite      equ     ((rdata     + 255) & ~255)                  ; sprite  location (256 byte align)
paltab      equ     (sprite     + (256*128))
heightmap   equ     (((paltab + 256) + 255) & ~255) 
bumpmap     equ     (heightmap  + (width *(height+2)))
sintab      equ     ((bumpmap    + (width *(height+2)*2) + 255) & ~255)

start:
        ; set mode 0x1 (40x25 color)
        
        ;mov         ah, 0x3
        inc         ax                      ; ax = 0 on startup
        ;mov         al, 0x13
        int         0x10
   
        ; sine table generator (by baze)
        ;xor        bx, bx    ; bx = 0 on startup

        mov         di, sintab
        inc         cx       ; cx = 0x00FF at startup
        mov         bp, 90   ; (512 * A * PI) / 256 = initial derivation
.store  mov         [di], bh
        inc         di
        mov         ax, 40    ; 262144 * (PI / 256)^2
        imul        bx
        add         bx, bp
        sub         bp, dx
        loop        .store
        
spritegen: 
 
        ; sprite generator
        mov         di, sprite
        mov         bx, 128
.yloop:
        mov         ch, (256 >> 8)                  ; cx = 0 from prev loop
.xloop:
        mov         ax, cx
        add         ax, -128
        mul         ax
        mov         bp, ax
        
        lea         ax, [bx-64]
        mul         ax
        inc         ax
        
        ; ax = x^2, dx = y^2
        add         bp, ax
        mov         ax, 0x1300
        cwd
        div         bp
        
        ; ax = 0x7FFF / (x^2 + y^2)
        ;test        ah, ah
        ;jz          .1
        ;mov         al, 0xFF
        
        cmp         ax, 0x7F
        jle         .1
        mov         al, 0x7F
.1:
        
        stosb
        loop        .xloop
    
        dec         bx
        jnz         .yloop
        
        ; bx = cx = 0
        
        ; palette
do_pal:
        mov         si, palette
        ;mov         di, paltab     ; di = paltab
.1:
        mov         cl, [si]
        jcxz        .2
        inc         si
        lodsb       
        rep         stosb
        jnz         .1
.2:
        
        
bumpgen:
        ; "landspace" gen
        ;mov         di, heightmap             ; di = heightmap from prev loop
        push        di
        mov         bl, height+2
.yloop:
        mov         cl, width
.xloop:
        in          al, 0x40
        mul         bx
        and         al, 4
        stosb
        loop        .xloop
        
        dec         bx
        jnz         .yloop
        
       
        pop         si
        
        mov         bp, sprite+((32 << 8) | (64))
difmap:
        mov         dx, height+2 
.yloop:
        mov         cl, width
.xloop:
        lodsb
        sub         al, [si-1-1]
        mov         ah, [si+width-1]
        sub         ah, [si-width-1]
        add         ax, bp
        stosw
        inc         bp
        loop        .xloop
        
        add         bp, (256 - width)
        dec         dx
        jnz         .yloop
        
        ; disable damn cursor
        mov         ah, 1
        mov         ch, 0x20
        int         0x10
        
%ifdef EGAVGA
        ; set high intensity
        mov         ax, 0x1003
        int         0x10
%endif      
      
        ; do the bump!       
        mov         dh, 0xB8            ; DX=0
        mov         es, dx
        xor         di, di
        mov         ch, 0x40
        mov         al, 0xB1
        rep         stosw
        
mainloop:
        push        bp
        
        ; get sintab position
        lea         bx, [bp+32]
        mov         bh, (sintab >> 8)
        add         bl, bl
        mov         ah, [bx]
        add         bl, bh
        add         bl, 0x32
        mov         al, [bx]
        add         ax, (12 << 8) | ((128-width) >> 1)
        mov         si, ax
        
        ; get sintab position
        mov         bx, bp
        mov         bh, (sintab >> 8)
        mov         dh, [bx]
        add         bl, bl
        add         bl, dl                      ; bl += 0xDA
        mov         dl, [bx]
        add         dx, (12 << 8) | ((128-width) >> 1)
        
        cli
        mov         bx, paltab
        mov         sp, bumpmap+width*2
        xor         di, di
        mov         cx, width*height
        
.1:
        ; ax = *bumpmap++;
        ; l = *(ax + disp);
        ; *screen++ = l;

        pop         bp
        mov         al, [bp+si]
        add         bp, dx
        add         al, [bp]
        
        xlatb
        inc         di
        stosb   
        and         al, 0x3
        or          al, 0x40
        out         0x61, al
        loop        .1
        
        pop         bp
        mov         dx, 0x3DA
    ;.retr2:
    ;    in          al, dx
    ;    and         al, 8
    ;    jnz         .retr2    
    .retr1:
        in          al, dx
        and         al, 8
        jz          .retr1

%ifndef EGAVGA
        ; set high intensity
        mov         dl, 0xD8        ; 0x3D8 - CGA mode register
        mov         al, 0x08
        out         dx, al
%endif  
        
        inc         bp
        
        in          al, 0x60
        dec         al
        jnz         mainloop
        
        ;int         0x20
        db          0xCD
        
palette:
%ifdef COMPOSITE
        ; length, color
        db 32, 0x00
        db  9, 0x08
        db  9, 0x09
        db 10, 0x19
        db 12, 0x99
        db 14, 0x9D
        db 18, 0xCC
        db 23 + 1  +128, 0xFF
%else
        ; length, color
        db 32, 0x00
        db  9, 0x08
        db  9, 0x09
        db 10, 0x19
        db 12, 0x99
        db 14, 0x9D
        db 18, 0xCC
        db 23 + 1 +128, 0xFF
%endif        
.count  equ ($ - palette)/2
        db 0
