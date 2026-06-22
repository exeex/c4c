# RV64 Aggregate Global Representative Recheck

## src/00024.c

status: emit-ok
asm: 1:    .bss
asm: 3:v:
asm: 4:    .zero 8
asm: 5:    .text
asm: 7:main:
asm: 10:    lla t0, v
asm: 11:    sw t1, 0(t0)
asm: 13:    lla t0, v
asm: 14:    sw t1, 4(t0)
asm: 15:    lla s1, v
asm: 16:    lw s1, 0(s1)
asm: 19:    lla s1, v
asm: 20:    lw s1, 4(s1)

## src/00047.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00048.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00050.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00091.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00115.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00146.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00148.c

status: emit-fail exit=1
diag: error: riscv prepared module emitter does not support this prepared global storage layout

## src/00163.c

status: emit-ok
asm: 16:    .bss
asm: 18:bolshevic:
asm: 19:    .zero 12
asm: 20:    .text
asm: 22:main:
asm: 27:    sw t1, 0(sp)
asm: 31:    lw t0, 0(sp)
asm: 33:remove:
asm: 35:rename:
asm: 37:renameat:
asm: 39:fclose:
asm: 41:tmpfile:
asm: 43:tmpnam:
asm: 45:tmpnam_r:
asm: 47:tempnam:
asm: 49:fflush:
asm: 51:fflush_unlocked:
asm: 53:fopen:
asm: 55:freopen:
asm: 57:fdopen:
asm: 59:fopencookie:
asm: 61:fmemopen:
asm: 63:open_memstream:
asm: 65:setbuf:
asm: 67:setvbuf:
asm: 69:setbuffer:
asm: 71:setlinebuf:
asm: 73:fprintf:
asm: 75:printf:
asm: 77:sprintf:
asm: 79:vfprintf:
asm: 81:vprintf:
asm: 83:vsprintf:
asm: 85:snprintf:
asm: 87:vsnprintf:
asm: 89:vasprintf:
asm: 91:__asprintf:
asm: 93:asprintf:
asm: 95:vdprintf:
asm: 97:dprintf:
asm: 99:fscanf:
asm: 101:scanf:
asm: 103:sscanf:
asm: 105:vfscanf:
asm: 107:vscanf:
asm: 109:vsscanf:
asm: 111:fgetc:
asm: 113:getc:
asm: 115:getchar:
asm: 117:getc_unlocked:
asm: 119:getchar_unlocked:
asm: 121:fgetc_unlocked:
asm: 123:fputc:
asm: 125:putc:
asm: 127:putchar:
asm: 129:fputc_unlocked:
asm: 131:putc_unlocked:
asm: 133:putchar_unlocked:
asm: 135:getw:
asm: 137:putw:
asm: 139:fgets:
asm: 141:__getdelim:
asm: 143:getdelim:
asm: 145:getline:
asm: 147:fputs:
asm: 149:puts:
asm: 151:ungetc:
asm: 153:fread:
asm: 155:fwrite:
asm: 157:fread_unlocked:
asm: 159:fwrite_unlocked:
asm: 161:fseek:
asm: 163:ftell:
asm: 165:rewind:
asm: 167:fseeko:
asm: 169:ftello:
asm: 171:fgetpos:
asm: 173:fsetpos:
asm: 175:clearerr:
asm: 177:feof:
asm: 179:ferror:
asm: 181:clearerr_unlocked:
asm: 183:feof_unlocked:
asm: 185:ferror_unlocked:
asm: 187:perror:
asm: 189:fileno:
asm: 191:fileno_unlocked:
asm: 193:pclose:
asm: 195:popen:
asm: 197:ctermid:
asm: 199:flockfile:
asm: 201:ftrylockfile:
asm: 203:funlockfile:
asm: 205:__uflow:
asm: 207:__overflow:

## src/00176.c

status: emit-ok
asm: 6:    .bss
asm: 8:array:
asm: 9:    .zero 64
asm: 10:    .text
asm: 12:swap:
asm: 17:partition:
asm: 22:    sw t1, 0(sp)
asm: 23:    lw t3, 0(sp)
asm: 24:    sw t3, 16(sp)
asm: 25:    lw t0, 16(sp)
asm: 27:quicksort:
asm: 32:main:
asm: 37:    lla t0, array
asm: 38:    sw t1, 0(t0)
asm: 40:    lla t0, array
asm: 41:    sw t1, 4(t0)
asm: 43:    lla t0, array
asm: 44:    sw t1, 8(t0)
asm: 46:    lla t0, array
asm: 47:    sw t1, 12(t0)
asm: 49:    lla t0, array
asm: 50:    sw t1, 16(t0)
asm: 52:    lla t0, array
asm: 53:    sw t1, 20(t0)
asm: 55:    lla t0, array
asm: 56:    sw t1, 24(t0)
asm: 58:    lla t0, array
asm: 59:    sw t1, 28(t0)
asm: 61:    lla t0, array
asm: 62:    sw t1, 32(t0)
asm: 64:    lla t0, array
asm: 65:    sw t1, 36(t0)
asm: 67:    lla t0, array
asm: 68:    sw t1, 40(t0)
asm: 70:    lla t0, array
asm: 71:    sw t1, 44(t0)
asm: 73:    lla t0, array
asm: 74:    sw t1, 48(t0)
asm: 76:    lla t0, array
asm: 77:    sw t1, 52(t0)
asm: 79:    lla t0, array
asm: 80:    sw t1, 56(t0)
asm: 82:    lla t0, array
asm: 83:    sw t1, 60(t0)
asm: 85:    sw t1, 0(sp)
asm: 88:    lw t0, 0(sp)
asm: 90:remove:
asm: 92:rename:
asm: 94:renameat:
asm: 96:fclose:
asm: 98:tmpfile:
asm: 100:tmpnam:
asm: 102:tmpnam_r:
asm: 104:tempnam:
asm: 106:fflush:
asm: 108:fflush_unlocked:
asm: 110:fopen:
asm: 112:freopen:
asm: 114:fdopen:
asm: 116:fopencookie:
asm: 118:fmemopen:
asm: 120:open_memstream:
asm: 122:setbuf:
asm: 124:setvbuf:
asm: 126:setbuffer:
asm: 128:setlinebuf:
asm: 130:fprintf:
asm: 132:printf:
asm: 134:sprintf:
asm: 136:vfprintf:
asm: 138:vprintf:
asm: 140:vsprintf:
asm: 142:snprintf:
asm: 144:vsnprintf:
asm: 146:vasprintf:
asm: 148:__asprintf:
asm: 150:asprintf:
asm: 152:vdprintf:
asm: 154:dprintf:
asm: 156:fscanf:
asm: 158:scanf:
asm: 160:sscanf:
asm: 162:vfscanf:
asm: 164:vscanf:
asm: 166:vsscanf:
asm: 168:fgetc:
asm: 170:getc:
asm: 172:getchar:
asm: 174:getc_unlocked:
asm: 176:getchar_unlocked:
asm: 178:fgetc_unlocked:
asm: 180:fputc:
asm: 182:putc:
asm: 184:putchar:
asm: 186:fputc_unlocked:
asm: 188:putc_unlocked:
asm: 190:putchar_unlocked:
asm: 192:getw:
asm: 194:putw:
asm: 196:fgets:
asm: 198:__getdelim:
asm: 200:getdelim:
asm: 202:getline:
asm: 204:fputs:
asm: 206:puts:
asm: 208:ungetc:
asm: 210:fread:
asm: 212:fwrite:
asm: 214:fread_unlocked:
asm: 216:fwrite_unlocked:
asm: 218:fseek:
asm: 220:ftell:
asm: 222:rewind:
asm: 224:fseeko:
asm: 226:ftello:
asm: 228:fgetpos:
asm: 230:fsetpos:
asm: 232:clearerr:
asm: 234:feof:
asm: 236:ferror:
asm: 238:clearerr_unlocked:
asm: 240:feof_unlocked:
asm: 242:ferror_unlocked:
asm: 244:perror:
asm: 246:fileno:
asm: 248:fileno_unlocked:
asm: 250:pclose:
asm: 252:popen:
asm: 254:ctermid:
asm: 256:flockfile:
asm: 258:ftrylockfile:
asm: 260:funlockfile:
asm: 262:__uflow:
asm: 264:__overflow:

