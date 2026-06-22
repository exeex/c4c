# RV64 String/Extern Representative Recheck

## src/00025.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 104, 101, 108, 108, 111, 0
asm: 4:    .text
asm: 6:main:
asm: 11:    lla t0, .str0
asm: 15:    call strlen

## src/00026.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 104, 101, 108, 108, 111, 0
asm: 4:    .text
asm: 6:main:
asm: 10:    lla s1, .str0

## src/00056.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 37, 100, 10, 0
asm: 4:.str1:
asm: 5:    .byte 37, 100, 44, 32, 37, 100, 10, 0
asm: 6:    .text
asm: 8:main:

## src/00058.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 97, 98, 99, 100, 101, 102, 0
asm: 4:    .text
asm: 6:main:
asm: 10:    lla s1, .str0

## src/00112.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 97, 98, 99, 0
asm: 4:    .text
asm: 6:main:

## src/00125.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 104, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 10, 0
asm: 4:    .text
asm: 6:main:

## src/00131.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 72, 101, 108, 108, 111, 10, 0
asm: 4:    .text
asm: 6:main:

## src/00132.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 72, 101, 108, 108, 111, 32, 119, 111, 114, 108, 100, 10, 0
asm: 4:.str1:
asm: 5:    .byte 67, 111, 117, 110, 116, 32, 61, 32, 37, 100, 10, 0
asm: 6:.str2:
asm: 7:    .byte 83, 116, 114, 105, 110, 103, 32, 39, 104, 101, 108, 108, 111, 39, 44, 32, 39, 116, 104, 101, 114, 101, 39, 32, 105, 115, 32, 39, 37, 115, 39, 44, 32, 39, 37, 115, 39, 10, 0
asm: 8:.str3:
asm: 9:    .byte 104, 101, 108, 108, 111, 0
asm: 10:.str4:
asm: 11:    .byte 116, 104, 101, 114, 101, 0
asm: 12:.str5:
asm: 13:    .byte 67, 104, 97, 114, 97, 99, 116, 101, 114, 32, 39, 65, 39, 32, 105, 115, 32, 39, 37, 99, 39, 10, 0
asm: 14:.str6:
asm: 15:    .byte 67, 104, 97, 114, 97, 99, 116, 101, 114, 32, 39, 97, 39, 32, 105, 115, 32, 39, 37, 99, 39, 10, 0
asm: 16:    .text
asm: 18:main:

## src/00137.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 104, 101, 108, 108, 111, 32, 105, 115, 32, 98, 101, 116, 116, 101, 114, 32, 116, 104, 97, 110, 32, 98, 121, 101, 0
asm: 4:    .text
asm: 6:main:
asm: 10:    lla s1, .str0

## src/00138.c

status: emit-ok
asm: 2:.str0:
asm: 3:    .byte 104, 105, 0
asm: 4:    .text
asm: 6:main:
asm: 10:    lla s1, .str0

