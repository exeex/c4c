target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }
%struct._anon_0 = type { [2 x i32] }
%struct._anon_2 = type { [4 x i8] }
%struct._anon_1 = type { i32, %struct._anon_2 }
%struct._G_fpos_t = type { i64, %struct._anon_1 }
%struct._G_fpos64_t = type { i64, %struct._anon_1 }
%struct._IO_marker = type {}
%struct._IO_FILE = type { i32, [4 x i8], ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, ptr, i32, i32, [1 x i8], [7 x i8], i64, i16, i8, [1 x i8], [4 x i8], ptr, i64, ptr, ptr, ptr, ptr, ptr, i32, [20 x i8] }
%struct._IO_codecvt = type {}
%struct._IO_wide_data = type {}
%struct._IO_cookie_io_functions_t = type { ptr, ptr, ptr, ptr }
%struct.s1 = type { [1 x i8] }
%struct.s2 = type { [2 x i8] }
%struct.s3 = type { [3 x i8] }
%struct.s4 = type { [4 x i8] }
%struct.s5 = type { [5 x i8] }
%struct.s6 = type { [6 x i8] }
%struct.s7 = type { [7 x i8] }
%struct.s8 = type { [8 x i8] }
%struct.s9 = type { [9 x i8] }
%struct.s10 = type { [10 x i8] }
%struct.s11 = type { [11 x i8] }
%struct.s12 = type { [12 x i8] }
%struct.s13 = type { [13 x i8] }
%struct.s14 = type { [14 x i8] }
%struct.s15 = type { [15 x i8] }
%struct.s16 = type { [16 x i8] }
%struct.s17 = type { [17 x i8] }
%struct.hfa11 = type { float }
%struct.hfa12 = type { float, float }
%struct.hfa13 = type { float, float, float }
%struct.hfa14 = type { float, float, float, float }
%struct.hfa21 = type { double }
%struct.hfa22 = type { double, double }
%struct.hfa23 = type { double, double, double }
%struct.hfa24 = type { double, double, double, double }
%struct.hfa31 = type { fp128 }
%struct.hfa32 = type { fp128, fp128 }
%struct.hfa33 = type { fp128, fp128, fp128 }
%struct.hfa34 = type { fp128, fp128, fp128, fp128 }
@.str0 = private unnamed_addr constant [6 x i8] c"%.1s\0A\00"
@.str1 = private unnamed_addr constant [6 x i8] c"%.2s\0A\00"
@.str2 = private unnamed_addr constant [6 x i8] c"%.3s\0A\00"
@.str3 = private unnamed_addr constant [6 x i8] c"%.4s\0A\00"
@.str4 = private unnamed_addr constant [6 x i8] c"%.5s\0A\00"
@.str5 = private unnamed_addr constant [6 x i8] c"%.6s\0A\00"
@.str6 = private unnamed_addr constant [6 x i8] c"%.7s\0A\00"
@.str7 = private unnamed_addr constant [6 x i8] c"%.8s\0A\00"
@.str8 = private unnamed_addr constant [6 x i8] c"%.9s\0A\00"
@.str9 = private unnamed_addr constant [7 x i8] c"%.10s\0A\00"
@.str10 = private unnamed_addr constant [7 x i8] c"%.11s\0A\00"
@.str11 = private unnamed_addr constant [7 x i8] c"%.12s\0A\00"
@.str12 = private unnamed_addr constant [7 x i8] c"%.13s\0A\00"
@.str13 = private unnamed_addr constant [7 x i8] c"%.14s\0A\00"
@.str14 = private unnamed_addr constant [7 x i8] c"%.15s\0A\00"
@.str15 = private unnamed_addr constant [7 x i8] c"%.16s\0A\00"
@.str16 = private unnamed_addr constant [7 x i8] c"%.17s\0A\00"
@.str17 = private unnamed_addr constant [6 x i8] c"%.1f\0A\00"
@.str18 = private unnamed_addr constant [11 x i8] c"%.1f %.1f\0A\00"
@.str19 = private unnamed_addr constant [16 x i8] c"%.1f %.1f %.1f\0A\00"
@.str20 = private unnamed_addr constant [21 x i8] c"%.1f %.1f %.1f %.1f\0A\00"
@.str21 = private unnamed_addr constant [7 x i8] c"%.1Lf\0A\00"
@.str22 = private unnamed_addr constant [13 x i8] c"%.1Lf %.1Lf\0A\00"
@.str23 = private unnamed_addr constant [19 x i8] c"%.1Lf %.1Lf %.1Lf\0A\00"
@.str24 = private unnamed_addr constant [25 x i8] c"%.1Lf %.1Lf %.1Lf %.1Lf\0A\00"
@.str25 = private unnamed_addr constant [31 x i8] c"%.3s %.3s %.3s %.3s %.3s %.3s\0A\00"
@.str26 = private unnamed_addr constant [33 x i8] c"%.1f %.1f %.1f %.1f %.1Lf %.1Lf\0A\00"
@.str27 = private unnamed_addr constant [48 x i8] c"%.1s %.1f %.1f %.2s %.1f %.1f %.3s %.1Lf %.1Lf\0A\00"
@.str28 = private unnamed_addr constant [12 x i8] c"Arguments:\0A\00"
@.str29 = private unnamed_addr constant [16 x i8] c"Return values:\0A\00"
@.str30 = private unnamed_addr constant [4 x i8] c"%7s\00"
@.str31 = private unnamed_addr constant [5 x i8] c"%.7s\00"
@.str32 = private unnamed_addr constant [4 x i8] c"%9s\00"
@.str33 = private unnamed_addr constant [5 x i8] c"%.9s\00"
@.str34 = private unnamed_addr constant [7 x i8] c"%hfa11\00"
@.str35 = private unnamed_addr constant [10 x i8] c"%.1f,%.1f\00"
@.str36 = private unnamed_addr constant [7 x i8] c"%hfa12\00"
@.str37 = private unnamed_addr constant [7 x i8] c"%hfa13\00"
@.str38 = private unnamed_addr constant [7 x i8] c"%hfa14\00"
@.str39 = private unnamed_addr constant [7 x i8] c"%hfa21\00"
@.str40 = private unnamed_addr constant [7 x i8] c"%hfa22\00"
@.str41 = private unnamed_addr constant [7 x i8] c"%hfa23\00"
@.str42 = private unnamed_addr constant [7 x i8] c"%hfa24\00"
@.str43 = private unnamed_addr constant [7 x i8] c"%hfa31\00"
@.str44 = private unnamed_addr constant [12 x i8] c"%.1Lf,%.1Lf\00"
@.str45 = private unnamed_addr constant [7 x i8] c"%hfa32\00"
@.str46 = private unnamed_addr constant [7 x i8] c"%hfa33\00"
@.str47 = private unnamed_addr constant [7 x i8] c"%hfa34\00"
@.str48 = private unnamed_addr constant [9 x i8] c"stdarg:\0A\00"
@.str49 = private unnamed_addr constant [24 x i8] c"%9s %9s %9s %9s %9s %9s\00"
@.str50 = private unnamed_addr constant [24 x i8] c"%7s %9s %9s %9s %9s %9s\00"
@.str51 = private unnamed_addr constant [17 x i8] c"HFA long double:\00"
@.str52 = private unnamed_addr constant [28 x i8] c"%hfa34 %hfa34 %hfa34 %hfa34\00"
@.str53 = private unnamed_addr constant [28 x i8] c"%hfa33 %hfa34 %hfa34 %hfa34\00"
@.str54 = private unnamed_addr constant [28 x i8] c"%hfa32 %hfa34 %hfa34 %hfa34\00"
@.str55 = private unnamed_addr constant [28 x i8] c"%hfa31 %hfa34 %hfa34 %hfa34\00"
@.str56 = private unnamed_addr constant [35 x i8] c"%hfa32 %hfa33 %hfa33 %hfa33 %hfa33\00"
@.str57 = private unnamed_addr constant [35 x i8] c"%hfa31 %hfa33 %hfa33 %hfa33 %hfa33\00"
@.str58 = private unnamed_addr constant [28 x i8] c"%hfa33 %hfa33 %hfa33 %hfa33\00"
@.str59 = private unnamed_addr constant [35 x i8] c"%hfa34 %hfa32 %hfa32 %hfa32 %hfa32\00"
@.str60 = private unnamed_addr constant [35 x i8] c"%hfa33 %hfa32 %hfa32 %hfa32 %hfa32\00"
@.str61 = private unnamed_addr constant [42 x i8] c"%hfa34 %hfa32 %hfa31 %hfa31 %hfa31 %hfa31\00"
@.str62 = private unnamed_addr constant [12 x i8] c"HFA double:\00"
@.str63 = private unnamed_addr constant [28 x i8] c"%hfa24 %hfa24 %hfa24 %hfa24\00"
@.str64 = private unnamed_addr constant [28 x i8] c"%hfa23 %hfa24 %hfa24 %hfa24\00"
@.str65 = private unnamed_addr constant [28 x i8] c"%hfa22 %hfa24 %hfa24 %hfa24\00"
@.str66 = private unnamed_addr constant [28 x i8] c"%hfa21 %hfa24 %hfa24 %hfa24\00"
@.str67 = private unnamed_addr constant [35 x i8] c"%hfa22 %hfa23 %hfa23 %hfa23 %hfa23\00"
@.str68 = private unnamed_addr constant [35 x i8] c"%hfa21 %hfa23 %hfa23 %hfa23 %hfa23\00"
@.str69 = private unnamed_addr constant [28 x i8] c"%hfa23 %hfa23 %hfa23 %hfa23\00"
@.str70 = private unnamed_addr constant [35 x i8] c"%hfa24 %hfa22 %hfa22 %hfa22 %hfa22\00"
@.str71 = private unnamed_addr constant [35 x i8] c"%hfa23 %hfa22 %hfa22 %hfa22 %hfa22\00"
@.str72 = private unnamed_addr constant [42 x i8] c"%hfa24 %hfa22 %hfa21 %hfa21 %hfa21 %hfa21\00"
@.str73 = private unnamed_addr constant [11 x i8] c"HFA float:\00"
@.str74 = private unnamed_addr constant [28 x i8] c"%hfa14 %hfa14 %hfa14 %hfa14\00"
@.str75 = private unnamed_addr constant [28 x i8] c"%hfa13 %hfa14 %hfa14 %hfa14\00"
@.str76 = private unnamed_addr constant [28 x i8] c"%hfa12 %hfa14 %hfa14 %hfa14\00"
@.str77 = private unnamed_addr constant [28 x i8] c"%hfa11 %hfa14 %hfa14 %hfa14\00"
@.str78 = private unnamed_addr constant [35 x i8] c"%hfa12 %hfa13 %hfa13 %hfa13 %hfa13\00"
@.str79 = private unnamed_addr constant [35 x i8] c"%hfa11 %hfa13 %hfa13 %hfa13 %hfa13\00"
@.str80 = private unnamed_addr constant [28 x i8] c"%hfa13 %hfa13 %hfa13 %hfa13\00"
@.str81 = private unnamed_addr constant [35 x i8] c"%hfa14 %hfa12 %hfa12 %hfa12 %hfa12\00"
@.str82 = private unnamed_addr constant [35 x i8] c"%hfa13 %hfa12 %hfa12 %hfa12 %hfa12\00"
@.str83 = private unnamed_addr constant [42 x i8] c"%hfa14 %hfa12 %hfa11 %hfa11 %hfa11 %hfa11\00"
@.str84 = private unnamed_addr constant [6 x i8] c"%llx\0A\00"
@.str85 = private unnamed_addr constant [7 x i8] c"MOVI:\0A\00"
@stdin = external global ptr, align 8
@stdout = external global ptr, align 8
@stderr = external global ptr, align 8
@s1 = global %struct.s1 { [1 x i8] c"0" }
@s2 = global %struct.s2 { [2 x i8] c"12" }
@s3 = global %struct.s3 { [3 x i8] c"345" }
@s4 = global %struct.s4 { [4 x i8] c"6789" }
@s5 = global %struct.s5 { [5 x i8] c"abcde" }
@s6 = global %struct.s6 { [6 x i8] c"fghijk" }
@s7 = global %struct.s7 { [7 x i8] c"lmnopqr" }
@s8 = global %struct.s8 { [8 x i8] c"stuvwxyz" }
@s9 = global %struct.s9 { [9 x i8] c"ABCDEFGHI" }
@s10 = global %struct.s10 { [10 x i8] c"JKLMNOPQRS" }
@s11 = global %struct.s11 { [11 x i8] c"TUVWXYZ0123" }
@s12 = global %struct.s12 { [12 x i8] c"456789abcdef" }
@s13 = global %struct.s13 { [13 x i8] c"ghijklmnopqrs" }
@s14 = global %struct.s14 { [14 x i8] c"tuvwxyzABCDEFG" }
@s15 = global %struct.s15 { [15 x i8] c"HIJKLMNOPQRSTUV" }
@s16 = global %struct.s16 { [16 x i8] c"WXYZ0123456789ab" }
@s17 = global %struct.s17 { [17 x i8] c"cdefghijklmnopqrs" }
@hfa11 = global %struct.hfa11 { float 0x4026333340000000 }, align 4
@hfa12 = global %struct.hfa12 { float 0x4028333340000000, float 0x4028666660000000 }, align 4
@hfa13 = global %struct.hfa13 { float 0x402A333340000000, float 0x402A666660000000, float 0x402A9999A0000000 }, align 4
@hfa14 = global %struct.hfa14 { float 0x402C333340000000, float 0x402C666660000000, float 0x402C9999A0000000, float 0x402CCCCCC0000000 }, align 4
@hfa21 = global %struct.hfa21 { double 0x403519999999999A }, align 8
@hfa22 = global %struct.hfa22 { double 0x403619999999999A, double 0x4036333333333333 }, align 8
@hfa23 = global %struct.hfa23 { double 0x403719999999999A, double 0x4037333333333333, double 0x40374CCCCCCCCCCD }, align 8
@hfa24 = global %struct.hfa24 { double 0x403819999999999A, double 0x4038333333333333, double 0x40384CCCCCCCCCCD, double 0x4038666666666666 }, align 8
@hfa31 = global %struct.hfa31 { fp128 0xLA0000000000000004003F19999999999 }, align 16
@hfa32 = global %struct.hfa32 { fp128 0xLD000000000000000400400CCCCCCCCCC, fp128 0xLA0000000000000004004019999999999 }, align 16
@hfa33 = global %struct.hfa33 { fp128 0xLD000000000000000400408CCCCCCCCCC, fp128 0xLA0000000000000004004099999999999, fp128 0xL600000000000000040040A6666666666 }, align 16
@hfa34 = global %struct.hfa34 { fp128 0xLD000000000000000400410CCCCCCCCCC, fp128 0xLA0000000000000004004119999999999, fp128 0xL60000000000000004004126666666666, fp128 0xL30000000000000004004133333333333 }, align 16

declare void @llvm.va_start.p0(ptr)
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

declare i32 @remove(ptr)

declare i32 @rename(ptr, ptr)

declare i32 @renameat(i32, ptr, i32, ptr)

declare i32 @fclose(ptr)

declare ptr @tmpfile()

declare ptr @tmpnam(ptr)

declare ptr @tmpnam_r(ptr)

declare ptr @tempnam(ptr, ptr)

declare i32 @fflush(ptr)

declare i32 @fflush_unlocked(ptr)

declare ptr @fopen(ptr, ptr)

declare ptr @freopen(ptr, ptr, ptr)

declare ptr @fdopen(i32, ptr)

declare ptr @fopencookie(ptr, ptr, %struct._IO_cookie_io_functions_t)

declare ptr @fmemopen(ptr, i64, ptr)

declare ptr @open_memstream(ptr, ptr)

declare void @setbuf(ptr, ptr)

declare i32 @setvbuf(ptr, ptr, i32, i64)

declare void @setbuffer(ptr, ptr, i64)

declare void @setlinebuf(ptr)

declare i32 @fprintf(ptr, ptr, ...)

declare i32 @printf(ptr, ...)

declare i32 @sprintf(ptr, ptr, ...)

declare i32 @vfprintf(ptr, ptr, ptr)

declare i32 @vprintf(ptr, ptr)

declare i32 @vsprintf(ptr, ptr, ptr)

declare i32 @snprintf(ptr, i64, ptr, ...)

declare i32 @vsnprintf(ptr, i64, ptr, ptr)

declare i32 @vasprintf(ptr, ptr, ptr)

declare i32 @__asprintf(ptr, ptr, ...)

declare i32 @asprintf(ptr, ptr, ...)

declare i32 @vdprintf(i32, ptr, ptr)

declare i32 @dprintf(i32, ptr, ...)

declare i32 @fscanf(ptr, ptr, ...)

declare i32 @scanf(ptr, ...)

declare i32 @sscanf(ptr, ptr, ...)

declare i32 @vfscanf(ptr, ptr, ptr)

declare i32 @vscanf(ptr, ptr)

declare i32 @vsscanf(ptr, ptr, ptr)

declare i32 @fgetc(ptr)

declare i32 @getc(ptr)

declare i32 @getchar()

declare i32 @getc_unlocked(ptr)

declare i32 @getchar_unlocked()

declare i32 @fgetc_unlocked(ptr)

declare i32 @fputc(i32, ptr)

declare i32 @putc(i32, ptr)

declare i32 @putchar(i32)

declare i32 @fputc_unlocked(i32, ptr)

declare i32 @putc_unlocked(i32, ptr)

declare i32 @putchar_unlocked(i32)

declare i32 @getw(ptr)

declare i32 @putw(i32, ptr)

declare ptr @fgets(ptr, i32, ptr)

declare i64 @__getdelim(ptr, ptr, i32, ptr)

declare i64 @getdelim(ptr, ptr, i32, ptr)

declare i64 @getline(ptr, ptr, ptr)

declare i32 @fputs(ptr, ptr)

declare i32 @puts(ptr)

declare i32 @ungetc(i32, ptr)

declare i64 @fread(ptr, i64, i64, ptr)

declare i64 @fwrite(ptr, i64, i64, ptr)

declare i64 @fread_unlocked(ptr, i64, i64, ptr)

declare i64 @fwrite_unlocked(ptr, i64, i64, ptr)

declare i32 @fseek(ptr, i64, i32)

declare i64 @ftell(ptr)

declare void @rewind(ptr)

declare i32 @fseeko(ptr, i64, i32)

declare i64 @ftello(ptr)

declare i32 @fgetpos(ptr, ptr)

declare i32 @fsetpos(ptr, ptr)

declare void @clearerr(ptr)

declare i32 @feof(ptr)

declare i32 @ferror(ptr)

declare void @clearerr_unlocked(ptr)

declare i32 @feof_unlocked(ptr)

declare i32 @ferror_unlocked(ptr)

declare void @perror(ptr)

declare i32 @fileno(ptr)

declare i32 @fileno_unlocked(ptr)

declare i32 @pclose(ptr)

declare ptr @popen(ptr, ptr)

declare ptr @ctermid(ptr)

declare void @flockfile(ptr)

declare i32 @ftrylockfile(ptr)

declare void @funlockfile(ptr)

declare i32 @__uflow(ptr)

declare i32 @__overflow(ptr, i32)

define void @fa_s1(%struct.s1 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s1
  store %struct.s1 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str0, i64 0, i64 0
  %t1 = getelementptr %struct.s1, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [1 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s2(%struct.s2 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s2
  store %struct.s2 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str1, i64 0, i64 0
  %t1 = getelementptr %struct.s2, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [2 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s3(%struct.s3 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s3
  store %struct.s3 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str2, i64 0, i64 0
  %t1 = getelementptr %struct.s3, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [3 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s4(%struct.s4 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s4
  store %struct.s4 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str3, i64 0, i64 0
  %t1 = getelementptr %struct.s4, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [4 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s5(%struct.s5 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s5
  store %struct.s5 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str4, i64 0, i64 0
  %t1 = getelementptr %struct.s5, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [5 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s6(%struct.s6 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s6
  store %struct.s6 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str5, i64 0, i64 0
  %t1 = getelementptr %struct.s6, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [6 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s7(%struct.s7 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s7
  store %struct.s7 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str6, i64 0, i64 0
  %t1 = getelementptr %struct.s7, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [7 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s8(%struct.s8 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s8
  store %struct.s8 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str7, i64 0, i64 0
  %t1 = getelementptr %struct.s8, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [8 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s9(%struct.s9 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s9
  store %struct.s9 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str8, i64 0, i64 0
  %t1 = getelementptr %struct.s9, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [9 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s10(%struct.s10 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s10
  store %struct.s10 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str9, i64 0, i64 0
  %t1 = getelementptr %struct.s10, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [10 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s11(%struct.s11 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s11
  store %struct.s11 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str10, i64 0, i64 0
  %t1 = getelementptr %struct.s11, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [11 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s12(%struct.s12 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s12
  store %struct.s12 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str11, i64 0, i64 0
  %t1 = getelementptr %struct.s12, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [12 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s13(%struct.s13 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s13
  store %struct.s13 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str12, i64 0, i64 0
  %t1 = getelementptr %struct.s13, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [13 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s14(%struct.s14 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s14
  store %struct.s14 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str13, i64 0, i64 0
  %t1 = getelementptr %struct.s14, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [14 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s15(%struct.s15 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s15
  store %struct.s15 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str14, i64 0, i64 0
  %t1 = getelementptr %struct.s15, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [15 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s16(%struct.s16 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s16
  store %struct.s16 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str15, i64 0, i64 0
  %t1 = getelementptr %struct.s16, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [16 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_s17(%struct.s17 %p.a)
{
entry:
  %lv.param.a = alloca %struct.s17
  store %struct.s17 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str16, i64 0, i64 0
  %t1 = getelementptr %struct.s17, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [17 x i8], ptr %t1, i64 0, i64 0
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2)
  ret void
}

define void @fa_hfa11(%struct.hfa11 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa11
  store %struct.hfa11 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str17, i64 0, i64 0
  %t1 = getelementptr %struct.hfa11, ptr %lv.param.a, i32 0, i32 0
  %t2 = load float, ptr %t1
  %t3 = fpext float %t2 to double
  %t4 = call i32 (ptr, ...) @printf(ptr %t0, double %t3)
  ret void
}

define void @fa_hfa12(%struct.hfa12 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa12
  store %struct.hfa12 %p.a, ptr %lv.param.a
  %t0 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t1 = getelementptr %struct.hfa12, ptr %lv.param.a, i32 0, i32 0
  %t2 = load float, ptr %t1
  %t3 = fpext float %t2 to double
  %t4 = getelementptr %struct.hfa12, ptr %lv.param.a, i32 0, i32 0
  %t5 = load float, ptr %t4
  %t6 = fpext float %t5 to double
  %t7 = call i32 (ptr, ...) @printf(ptr %t0, double %t3, double %t6)
  ret void
}

define void @fa_hfa13(%struct.hfa13 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa13
  store %struct.hfa13 %p.a, ptr %lv.param.a
  %t0 = getelementptr [16 x i8], ptr @.str19, i64 0, i64 0
  %t1 = getelementptr %struct.hfa13, ptr %lv.param.a, i32 0, i32 0
  %t2 = load float, ptr %t1
  %t3 = fpext float %t2 to double
  %t4 = getelementptr %struct.hfa13, ptr %lv.param.a, i32 0, i32 1
  %t5 = load float, ptr %t4
  %t6 = fpext float %t5 to double
  %t7 = getelementptr %struct.hfa13, ptr %lv.param.a, i32 0, i32 2
  %t8 = load float, ptr %t7
  %t9 = fpext float %t8 to double
  %t10 = call i32 (ptr, ...) @printf(ptr %t0, double %t3, double %t6, double %t9)
  ret void
}

define void @fa_hfa14(%struct.hfa14 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa14
  store %struct.hfa14 %p.a, ptr %lv.param.a
  %t0 = getelementptr [21 x i8], ptr @.str20, i64 0, i64 0
  %t1 = getelementptr %struct.hfa14, ptr %lv.param.a, i32 0, i32 0
  %t2 = load float, ptr %t1
  %t3 = fpext float %t2 to double
  %t4 = getelementptr %struct.hfa14, ptr %lv.param.a, i32 0, i32 1
  %t5 = load float, ptr %t4
  %t6 = fpext float %t5 to double
  %t7 = getelementptr %struct.hfa14, ptr %lv.param.a, i32 0, i32 2
  %t8 = load float, ptr %t7
  %t9 = fpext float %t8 to double
  %t10 = getelementptr %struct.hfa14, ptr %lv.param.a, i32 0, i32 3
  %t11 = load float, ptr %t10
  %t12 = fpext float %t11 to double
  %t13 = call i32 (ptr, ...) @printf(ptr %t0, double %t3, double %t6, double %t9, double %t12)
  ret void
}

define void @fa_hfa21(%struct.hfa21 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa21
  store %struct.hfa21 %p.a, ptr %lv.param.a
  %t0 = getelementptr [6 x i8], ptr @.str17, i64 0, i64 0
  %t1 = getelementptr %struct.hfa21, ptr %lv.param.a, i32 0, i32 0
  %t2 = load double, ptr %t1
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, double %t2)
  ret void
}

define void @fa_hfa22(%struct.hfa22 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa22
  store %struct.hfa22 %p.a, ptr %lv.param.a
  %t0 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t1 = getelementptr %struct.hfa22, ptr %lv.param.a, i32 0, i32 0
  %t2 = load double, ptr %t1
  %t3 = getelementptr %struct.hfa22, ptr %lv.param.a, i32 0, i32 0
  %t4 = load double, ptr %t3
  %t5 = call i32 (ptr, ...) @printf(ptr %t0, double %t2, double %t4)
  ret void
}

define void @fa_hfa23(%struct.hfa23 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa23
  store %struct.hfa23 %p.a, ptr %lv.param.a
  %t0 = getelementptr [16 x i8], ptr @.str19, i64 0, i64 0
  %t1 = getelementptr %struct.hfa23, ptr %lv.param.a, i32 0, i32 0
  %t2 = load double, ptr %t1
  %t3 = getelementptr %struct.hfa23, ptr %lv.param.a, i32 0, i32 1
  %t4 = load double, ptr %t3
  %t5 = getelementptr %struct.hfa23, ptr %lv.param.a, i32 0, i32 2
  %t6 = load double, ptr %t5
  %t7 = call i32 (ptr, ...) @printf(ptr %t0, double %t2, double %t4, double %t6)
  ret void
}

define void @fa_hfa24(%struct.hfa24 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa24
  store %struct.hfa24 %p.a, ptr %lv.param.a
  %t0 = getelementptr [21 x i8], ptr @.str20, i64 0, i64 0
  %t1 = getelementptr %struct.hfa24, ptr %lv.param.a, i32 0, i32 0
  %t2 = load double, ptr %t1
  %t3 = getelementptr %struct.hfa24, ptr %lv.param.a, i32 0, i32 1
  %t4 = load double, ptr %t3
  %t5 = getelementptr %struct.hfa24, ptr %lv.param.a, i32 0, i32 2
  %t6 = load double, ptr %t5
  %t7 = getelementptr %struct.hfa24, ptr %lv.param.a, i32 0, i32 3
  %t8 = load double, ptr %t7
  %t9 = call i32 (ptr, ...) @printf(ptr %t0, double %t2, double %t4, double %t6, double %t8)
  ret void
}

define void @fa_hfa31(%struct.hfa31 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa31
  store %struct.hfa31 %p.a, ptr %lv.param.a
  %t0 = getelementptr [7 x i8], ptr @.str21, i64 0, i64 0
  %t1 = getelementptr %struct.hfa31, ptr %lv.param.a, i32 0, i32 0
  %t2 = load fp128, ptr %t1
  %t3 = call i32 (ptr, ...) @printf(ptr %t0, fp128 %t2)
  ret void
}

define void @fa_hfa32(%struct.hfa32 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa32
  store %struct.hfa32 %p.a, ptr %lv.param.a
  %t0 = getelementptr [13 x i8], ptr @.str22, i64 0, i64 0
  %t1 = getelementptr %struct.hfa32, ptr %lv.param.a, i32 0, i32 0
  %t2 = load fp128, ptr %t1
  %t3 = getelementptr %struct.hfa32, ptr %lv.param.a, i32 0, i32 0
  %t4 = load fp128, ptr %t3
  %t5 = call i32 (ptr, ...) @printf(ptr %t0, fp128 %t2, fp128 %t4)
  ret void
}

define void @fa_hfa33(%struct.hfa33 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa33
  store %struct.hfa33 %p.a, ptr %lv.param.a
  %t0 = getelementptr [19 x i8], ptr @.str23, i64 0, i64 0
  %t1 = getelementptr %struct.hfa33, ptr %lv.param.a, i32 0, i32 0
  %t2 = load fp128, ptr %t1
  %t3 = getelementptr %struct.hfa33, ptr %lv.param.a, i32 0, i32 1
  %t4 = load fp128, ptr %t3
  %t5 = getelementptr %struct.hfa33, ptr %lv.param.a, i32 0, i32 2
  %t6 = load fp128, ptr %t5
  %t7 = call i32 (ptr, ...) @printf(ptr %t0, fp128 %t2, fp128 %t4, fp128 %t6)
  ret void
}

define void @fa_hfa34(%struct.hfa34 %p.a)
{
entry:
  %lv.param.a = alloca %struct.hfa34
  store %struct.hfa34 %p.a, ptr %lv.param.a
  %t0 = getelementptr [25 x i8], ptr @.str24, i64 0, i64 0
  %t1 = getelementptr %struct.hfa34, ptr %lv.param.a, i32 0, i32 0
  %t2 = load fp128, ptr %t1
  %t3 = getelementptr %struct.hfa34, ptr %lv.param.a, i32 0, i32 1
  %t4 = load fp128, ptr %t3
  %t5 = getelementptr %struct.hfa34, ptr %lv.param.a, i32 0, i32 2
  %t6 = load fp128, ptr %t5
  %t7 = getelementptr %struct.hfa34, ptr %lv.param.a, i32 0, i32 3
  %t8 = load fp128, ptr %t7
  %t9 = call i32 (ptr, ...) @printf(ptr %t0, fp128 %t2, fp128 %t4, fp128 %t6, fp128 %t8)
  ret void
}

define void @fa1(%struct.s8 %p.a, %struct.s9 %p.b, %struct.s10 %p.c, %struct.s11 %p.d, %struct.s12 %p.e, %struct.s13 %p.f)
{
entry:
  %lv.param.a = alloca %struct.s8
  store %struct.s8 %p.a, ptr %lv.param.a
  %lv.param.b = alloca %struct.s9
  store %struct.s9 %p.b, ptr %lv.param.b
  %lv.param.c = alloca %struct.s10
  store %struct.s10 %p.c, ptr %lv.param.c
  %lv.param.d = alloca %struct.s11
  store %struct.s11 %p.d, ptr %lv.param.d
  %lv.param.e = alloca %struct.s12
  store %struct.s12 %p.e, ptr %lv.param.e
  %lv.param.f = alloca %struct.s13
  store %struct.s13 %p.f, ptr %lv.param.f
  %t0 = getelementptr [31 x i8], ptr @.str25, i64 0, i64 0
  %t1 = getelementptr %struct.s8, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [8 x i8], ptr %t1, i64 0, i64 0
  %t3 = getelementptr %struct.s9, ptr %lv.param.b, i32 0, i32 0
  %t4 = getelementptr [9 x i8], ptr %t3, i64 0, i64 0
  %t5 = getelementptr %struct.s10, ptr %lv.param.c, i32 0, i32 0
  %t6 = getelementptr [10 x i8], ptr %t5, i64 0, i64 0
  %t7 = getelementptr %struct.s11, ptr %lv.param.d, i32 0, i32 0
  %t8 = getelementptr [11 x i8], ptr %t7, i64 0, i64 0
  %t9 = getelementptr %struct.s12, ptr %lv.param.e, i32 0, i32 0
  %t10 = getelementptr [12 x i8], ptr %t9, i64 0, i64 0
  %t11 = getelementptr %struct.s13, ptr %lv.param.f, i32 0, i32 0
  %t12 = getelementptr [13 x i8], ptr %t11, i64 0, i64 0
  %t13 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2, ptr %t4, ptr %t6, ptr %t8, ptr %t10, ptr %t12)
  ret void
}

define void @fa2(%struct.s9 %p.a, %struct.s10 %p.b, %struct.s11 %p.c, %struct.s12 %p.d, %struct.s13 %p.e, %struct.s14 %p.f)
{
entry:
  %lv.param.a = alloca %struct.s9
  store %struct.s9 %p.a, ptr %lv.param.a
  %lv.param.b = alloca %struct.s10
  store %struct.s10 %p.b, ptr %lv.param.b
  %lv.param.c = alloca %struct.s11
  store %struct.s11 %p.c, ptr %lv.param.c
  %lv.param.d = alloca %struct.s12
  store %struct.s12 %p.d, ptr %lv.param.d
  %lv.param.e = alloca %struct.s13
  store %struct.s13 %p.e, ptr %lv.param.e
  %lv.param.f = alloca %struct.s14
  store %struct.s14 %p.f, ptr %lv.param.f
  %t0 = getelementptr [31 x i8], ptr @.str25, i64 0, i64 0
  %t1 = getelementptr %struct.s9, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [9 x i8], ptr %t1, i64 0, i64 0
  %t3 = getelementptr %struct.s10, ptr %lv.param.b, i32 0, i32 0
  %t4 = getelementptr [10 x i8], ptr %t3, i64 0, i64 0
  %t5 = getelementptr %struct.s11, ptr %lv.param.c, i32 0, i32 0
  %t6 = getelementptr [11 x i8], ptr %t5, i64 0, i64 0
  %t7 = getelementptr %struct.s12, ptr %lv.param.d, i32 0, i32 0
  %t8 = getelementptr [12 x i8], ptr %t7, i64 0, i64 0
  %t9 = getelementptr %struct.s13, ptr %lv.param.e, i32 0, i32 0
  %t10 = getelementptr [13 x i8], ptr %t9, i64 0, i64 0
  %t11 = getelementptr %struct.s14, ptr %lv.param.f, i32 0, i32 0
  %t12 = getelementptr [14 x i8], ptr %t11, i64 0, i64 0
  %t13 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2, ptr %t4, ptr %t6, ptr %t8, ptr %t10, ptr %t12)
  ret void
}

define void @fa3(%struct.hfa14 %p.a, %struct.hfa23 %p.b, %struct.hfa32 %p.c)
{
entry:
  %lv.param.a = alloca %struct.hfa14
  store %struct.hfa14 %p.a, ptr %lv.param.a
  %lv.param.b = alloca %struct.hfa23
  store %struct.hfa23 %p.b, ptr %lv.param.b
  %lv.param.c = alloca %struct.hfa32
  store %struct.hfa32 %p.c, ptr %lv.param.c
  %t0 = getelementptr [33 x i8], ptr @.str26, i64 0, i64 0
  %t1 = getelementptr %struct.hfa14, ptr %lv.param.a, i32 0, i32 0
  %t2 = load float, ptr %t1
  %t3 = fpext float %t2 to double
  %t4 = getelementptr %struct.hfa14, ptr %lv.param.a, i32 0, i32 3
  %t5 = load float, ptr %t4
  %t6 = fpext float %t5 to double
  %t7 = getelementptr %struct.hfa23, ptr %lv.param.b, i32 0, i32 0
  %t8 = load double, ptr %t7
  %t9 = getelementptr %struct.hfa23, ptr %lv.param.b, i32 0, i32 2
  %t10 = load double, ptr %t9
  %t11 = getelementptr %struct.hfa32, ptr %lv.param.c, i32 0, i32 0
  %t12 = load fp128, ptr %t11
  %t13 = getelementptr %struct.hfa32, ptr %lv.param.c, i32 0, i32 1
  %t14 = load fp128, ptr %t13
  %t15 = call i32 (ptr, ...) @printf(ptr %t0, double %t3, double %t6, double %t8, double %t10, fp128 %t12, fp128 %t14)
  ret void
}

define void @fa4(%struct.s1 %p.a, %struct.hfa14 %p.b, %struct.s2 %p.c, %struct.hfa24 %p.d, %struct.s3 %p.e, %struct.hfa34 %p.f)
{
entry:
  %lv.param.a = alloca %struct.s1
  store %struct.s1 %p.a, ptr %lv.param.a
  %lv.param.b = alloca %struct.hfa14
  store %struct.hfa14 %p.b, ptr %lv.param.b
  %lv.param.c = alloca %struct.s2
  store %struct.s2 %p.c, ptr %lv.param.c
  %lv.param.d = alloca %struct.hfa24
  store %struct.hfa24 %p.d, ptr %lv.param.d
  %lv.param.e = alloca %struct.s3
  store %struct.s3 %p.e, ptr %lv.param.e
  %lv.param.f = alloca %struct.hfa34
  store %struct.hfa34 %p.f, ptr %lv.param.f
  %t0 = getelementptr [48 x i8], ptr @.str27, i64 0, i64 0
  %t1 = getelementptr %struct.s1, ptr %lv.param.a, i32 0, i32 0
  %t2 = getelementptr [1 x i8], ptr %t1, i64 0, i64 0
  %t3 = getelementptr %struct.hfa14, ptr %lv.param.b, i32 0, i32 0
  %t4 = load float, ptr %t3
  %t5 = fpext float %t4 to double
  %t6 = getelementptr %struct.hfa14, ptr %lv.param.b, i32 0, i32 3
  %t7 = load float, ptr %t6
  %t8 = fpext float %t7 to double
  %t9 = getelementptr %struct.s2, ptr %lv.param.c, i32 0, i32 0
  %t10 = getelementptr [2 x i8], ptr %t9, i64 0, i64 0
  %t11 = getelementptr %struct.hfa24, ptr %lv.param.d, i32 0, i32 0
  %t12 = load double, ptr %t11
  %t13 = getelementptr %struct.hfa24, ptr %lv.param.d, i32 0, i32 3
  %t14 = load double, ptr %t13
  %t15 = getelementptr %struct.s3, ptr %lv.param.e, i32 0, i32 0
  %t16 = getelementptr [3 x i8], ptr %t15, i64 0, i64 0
  %t17 = getelementptr %struct.hfa34, ptr %lv.param.f, i32 0, i32 0
  %t18 = load fp128, ptr %t17
  %t19 = getelementptr %struct.hfa34, ptr %lv.param.f, i32 0, i32 3
  %t20 = load fp128, ptr %t19
  %t21 = call i32 (ptr, ...) @printf(ptr %t0, ptr %t2, double %t5, double %t8, ptr %t10, double %t12, double %t14, ptr %t16, fp128 %t18, fp128 %t20)
  ret void
}

define void @arg()
{
entry:
  %t0 = getelementptr [12 x i8], ptr @.str28, i64 0, i64 0
  %t1 = call i32 (ptr, ...) @printf(ptr %t0)
  %t2 = load %struct.s1, ptr @s1
  call void (%struct.s1) @fa_s1(%struct.s1 %t2)
  %t3 = load %struct.s2, ptr @s2
  call void (%struct.s2) @fa_s2(%struct.s2 %t3)
  %t4 = load %struct.s3, ptr @s3
  call void (%struct.s3) @fa_s3(%struct.s3 %t4)
  %t5 = load %struct.s4, ptr @s4
  call void (%struct.s4) @fa_s4(%struct.s4 %t5)
  %t6 = load %struct.s5, ptr @s5
  call void (%struct.s5) @fa_s5(%struct.s5 %t6)
  %t7 = load %struct.s6, ptr @s6
  call void (%struct.s6) @fa_s6(%struct.s6 %t7)
  %t8 = load %struct.s7, ptr @s7
  call void (%struct.s7) @fa_s7(%struct.s7 %t8)
  %t9 = load %struct.s8, ptr @s8
  call void (%struct.s8) @fa_s8(%struct.s8 %t9)
  %t10 = load %struct.s9, ptr @s9
  call void (%struct.s9) @fa_s9(%struct.s9 %t10)
  %t11 = load %struct.s10, ptr @s10
  call void (%struct.s10) @fa_s10(%struct.s10 %t11)
  %t12 = load %struct.s11, ptr @s11
  call void (%struct.s11) @fa_s11(%struct.s11 %t12)
  %t13 = load %struct.s12, ptr @s12
  call void (%struct.s12) @fa_s12(%struct.s12 %t13)
  %t14 = load %struct.s13, ptr @s13
  call void (%struct.s13) @fa_s13(%struct.s13 %t14)
  %t15 = load %struct.s14, ptr @s14
  call void (%struct.s14) @fa_s14(%struct.s14 %t15)
  %t16 = load %struct.s15, ptr @s15
  call void (%struct.s15) @fa_s15(%struct.s15 %t16)
  %t17 = load %struct.s16, ptr @s16
  call void (%struct.s16) @fa_s16(%struct.s16 %t17)
  %t18 = load %struct.s17, ptr @s17
  call void (%struct.s17) @fa_s17(%struct.s17 %t18)
  %t19 = load %struct.hfa11, ptr @hfa11
  call void (%struct.hfa11) @fa_hfa11(%struct.hfa11 %t19)
  %t20 = load %struct.hfa12, ptr @hfa12
  call void (%struct.hfa12) @fa_hfa12(%struct.hfa12 %t20)
  %t21 = load %struct.hfa13, ptr @hfa13
  call void (%struct.hfa13) @fa_hfa13(%struct.hfa13 %t21)
  %t22 = load %struct.hfa14, ptr @hfa14
  call void (%struct.hfa14) @fa_hfa14(%struct.hfa14 %t22)
  %t23 = load %struct.hfa21, ptr @hfa21
  call void (%struct.hfa21) @fa_hfa21(%struct.hfa21 %t23)
  %t24 = load %struct.hfa22, ptr @hfa22
  call void (%struct.hfa22) @fa_hfa22(%struct.hfa22 %t24)
  %t25 = load %struct.hfa23, ptr @hfa23
  call void (%struct.hfa23) @fa_hfa23(%struct.hfa23 %t25)
  %t26 = load %struct.hfa24, ptr @hfa24
  call void (%struct.hfa24) @fa_hfa24(%struct.hfa24 %t26)
  %t27 = load %struct.hfa31, ptr @hfa31
  call void (%struct.hfa31) @fa_hfa31(%struct.hfa31 %t27)
  %t28 = load %struct.hfa32, ptr @hfa32
  call void (%struct.hfa32) @fa_hfa32(%struct.hfa32 %t28)
  %t29 = load %struct.hfa33, ptr @hfa33
  call void (%struct.hfa33) @fa_hfa33(%struct.hfa33 %t29)
  %t30 = load %struct.hfa34, ptr @hfa34
  call void (%struct.hfa34) @fa_hfa34(%struct.hfa34 %t30)
  %t31 = load %struct.s8, ptr @s8
  %t32 = load %struct.s9, ptr @s9
  %t33 = load %struct.s10, ptr @s10
  %t34 = load %struct.s11, ptr @s11
  %t35 = load %struct.s12, ptr @s12
  %t36 = load %struct.s13, ptr @s13
  call void (%struct.s8, %struct.s9, %struct.s10, %struct.s11, %struct.s12, %struct.s13) @fa1(%struct.s8 %t31, %struct.s9 %t32, %struct.s10 %t33, %struct.s11 %t34, %struct.s12 %t35, %struct.s13 %t36)
  %t37 = load %struct.s9, ptr @s9
  %t38 = load %struct.s10, ptr @s10
  %t39 = load %struct.s11, ptr @s11
  %t40 = load %struct.s12, ptr @s12
  %t41 = load %struct.s13, ptr @s13
  %t42 = load %struct.s14, ptr @s14
  call void (%struct.s9, %struct.s10, %struct.s11, %struct.s12, %struct.s13, %struct.s14) @fa2(%struct.s9 %t37, %struct.s10 %t38, %struct.s11 %t39, %struct.s12 %t40, %struct.s13 %t41, %struct.s14 %t42)
  %t43 = load %struct.hfa14, ptr @hfa14
  %t44 = load %struct.hfa23, ptr @hfa23
  %t45 = load %struct.hfa32, ptr @hfa32
  call void (%struct.hfa14, %struct.hfa23, %struct.hfa32) @fa3(%struct.hfa14 %t43, %struct.hfa23 %t44, %struct.hfa32 %t45)
  %t46 = load %struct.s1, ptr @s1
  %t47 = load %struct.hfa14, ptr @hfa14
  %t48 = load %struct.s2, ptr @s2
  %t49 = load %struct.hfa24, ptr @hfa24
  %t50 = load %struct.s3, ptr @s3
  %t51 = load %struct.hfa34, ptr @hfa34
  call void (%struct.s1, %struct.hfa14, %struct.s2, %struct.hfa24, %struct.s3, %struct.hfa34) @fa4(%struct.s1 %t46, %struct.hfa14 %t47, %struct.s2 %t48, %struct.hfa24 %t49, %struct.s3 %t50, %struct.hfa34 %t51)
  ret void
}

define %struct.s1 @fr_s1()
{
entry:
  %t0 = load %struct.s1, ptr @s1
  ret %struct.s1 %t0
}

define %struct.s2 @fr_s2()
{
entry:
  %t0 = load %struct.s2, ptr @s2
  ret %struct.s2 %t0
}

define %struct.s3 @fr_s3()
{
entry:
  %t0 = load %struct.s3, ptr @s3
  ret %struct.s3 %t0
}

define %struct.s4 @fr_s4()
{
entry:
  %t0 = load %struct.s4, ptr @s4
  ret %struct.s4 %t0
}

define %struct.s5 @fr_s5()
{
entry:
  %t0 = load %struct.s5, ptr @s5
  ret %struct.s5 %t0
}

define %struct.s6 @fr_s6()
{
entry:
  %t0 = load %struct.s6, ptr @s6
  ret %struct.s6 %t0
}

define %struct.s7 @fr_s7()
{
entry:
  %t0 = load %struct.s7, ptr @s7
  ret %struct.s7 %t0
}

define %struct.s8 @fr_s8()
{
entry:
  %t0 = load %struct.s8, ptr @s8
  ret %struct.s8 %t0
}

define %struct.s9 @fr_s9()
{
entry:
  %t0 = load %struct.s9, ptr @s9
  ret %struct.s9 %t0
}

define %struct.s10 @fr_s10()
{
entry:
  %t0 = load %struct.s10, ptr @s10
  ret %struct.s10 %t0
}

define %struct.s11 @fr_s11()
{
entry:
  %t0 = load %struct.s11, ptr @s11
  ret %struct.s11 %t0
}

define %struct.s12 @fr_s12()
{
entry:
  %t0 = load %struct.s12, ptr @s12
  ret %struct.s12 %t0
}

define %struct.s13 @fr_s13()
{
entry:
  %t0 = load %struct.s13, ptr @s13
  ret %struct.s13 %t0
}

define %struct.s14 @fr_s14()
{
entry:
  %t0 = load %struct.s14, ptr @s14
  ret %struct.s14 %t0
}

define %struct.s15 @fr_s15()
{
entry:
  %t0 = load %struct.s15, ptr @s15
  ret %struct.s15 %t0
}

define %struct.s16 @fr_s16()
{
entry:
  %t0 = load %struct.s16, ptr @s16
  ret %struct.s16 %t0
}

define %struct.s17 @fr_s17()
{
entry:
  %t0 = load %struct.s17, ptr @s17
  ret %struct.s17 %t0
}

define %struct.hfa11 @fr_hfa11()
{
entry:
  %t0 = load %struct.hfa11, ptr @hfa11
  ret %struct.hfa11 %t0
}

define %struct.hfa12 @fr_hfa12()
{
entry:
  %t0 = load %struct.hfa12, ptr @hfa12
  ret %struct.hfa12 %t0
}

define %struct.hfa13 @fr_hfa13()
{
entry:
  %t0 = load %struct.hfa13, ptr @hfa13
  ret %struct.hfa13 %t0
}

define %struct.hfa14 @fr_hfa14()
{
entry:
  %t0 = load %struct.hfa14, ptr @hfa14
  ret %struct.hfa14 %t0
}

define %struct.hfa21 @fr_hfa21()
{
entry:
  %t0 = load %struct.hfa21, ptr @hfa21
  ret %struct.hfa21 %t0
}

define %struct.hfa22 @fr_hfa22()
{
entry:
  %t0 = load %struct.hfa22, ptr @hfa22
  ret %struct.hfa22 %t0
}

define %struct.hfa23 @fr_hfa23()
{
entry:
  %t0 = load %struct.hfa23, ptr @hfa23
  ret %struct.hfa23 %t0
}

define %struct.hfa24 @fr_hfa24()
{
entry:
  %t0 = load %struct.hfa24, ptr @hfa24
  ret %struct.hfa24 %t0
}

define %struct.hfa31 @fr_hfa31()
{
entry:
  %t0 = load %struct.hfa31, ptr @hfa31
  ret %struct.hfa31 %t0
}

define %struct.hfa32 @fr_hfa32()
{
entry:
  %t0 = load %struct.hfa32, ptr @hfa32
  ret %struct.hfa32 %t0
}

define %struct.hfa33 @fr_hfa33()
{
entry:
  %t0 = load %struct.hfa33, ptr @hfa33
  ret %struct.hfa33 %t0
}

define %struct.hfa34 @fr_hfa34()
{
entry:
  %t0 = load %struct.hfa34, ptr @hfa34
  ret %struct.hfa34 %t0
}

define void @ret()
{
entry:
  %lv.t1 = alloca %struct.s1, align 1
  store %struct.s1 zeroinitializer, ptr %lv.t1
  %lv.t2 = alloca %struct.s2, align 1
  store %struct.s2 zeroinitializer, ptr %lv.t2
  %lv.t3 = alloca %struct.s3, align 1
  store %struct.s3 zeroinitializer, ptr %lv.t3
  %lv.t4 = alloca %struct.s4, align 1
  store %struct.s4 zeroinitializer, ptr %lv.t4
  %lv.t5 = alloca %struct.s5, align 1
  store %struct.s5 zeroinitializer, ptr %lv.t5
  %lv.t6 = alloca %struct.s6, align 1
  store %struct.s6 zeroinitializer, ptr %lv.t6
  %lv.t7 = alloca %struct.s7, align 1
  store %struct.s7 zeroinitializer, ptr %lv.t7
  %lv.t8 = alloca %struct.s8, align 1
  store %struct.s8 zeroinitializer, ptr %lv.t8
  %lv.t9 = alloca %struct.s9, align 1
  store %struct.s9 zeroinitializer, ptr %lv.t9
  %lv.t10 = alloca %struct.s10, align 1
  store %struct.s10 zeroinitializer, ptr %lv.t10
  %lv.t11 = alloca %struct.s11, align 1
  store %struct.s11 zeroinitializer, ptr %lv.t11
  %lv.t12 = alloca %struct.s12, align 1
  store %struct.s12 zeroinitializer, ptr %lv.t12
  %lv.t13 = alloca %struct.s13, align 1
  store %struct.s13 zeroinitializer, ptr %lv.t13
  %lv.t14 = alloca %struct.s14, align 1
  store %struct.s14 zeroinitializer, ptr %lv.t14
  %lv.t15 = alloca %struct.s15, align 1
  store %struct.s15 zeroinitializer, ptr %lv.t15
  %lv.t16 = alloca %struct.s16, align 1
  store %struct.s16 zeroinitializer, ptr %lv.t16
  %lv.t17 = alloca %struct.s17, align 1
  store %struct.s17 zeroinitializer, ptr %lv.t17
  %t89.agg = alloca %struct.hfa11
  %t96.agg = alloca %struct.hfa12
  %t101.agg = alloca %struct.hfa12
  %t108.agg = alloca %struct.hfa13
  %t113.agg = alloca %struct.hfa13
  %t120.agg = alloca %struct.hfa14
  %t125.agg = alloca %struct.hfa14
  %t132.agg = alloca %struct.hfa21
  %t138.agg = alloca %struct.hfa22
  %t142.agg = alloca %struct.hfa22
  %t148.agg = alloca %struct.hfa23
  %t152.agg = alloca %struct.hfa23
  %t158.agg = alloca %struct.hfa24
  %t162.agg = alloca %struct.hfa24
  %t168.agg = alloca %struct.hfa31
  %t174.agg = alloca %struct.hfa32
  %t178.agg = alloca %struct.hfa32
  %t184.agg = alloca %struct.hfa33
  %t188.agg = alloca %struct.hfa33
  %t194.agg = alloca %struct.hfa34
  %t198.agg = alloca %struct.hfa34
  %t0 = call %struct.s1 () @fr_s1()
  store %struct.s1 %t0, ptr %lv.t1
  %t1 = call %struct.s2 () @fr_s2()
  store %struct.s2 %t1, ptr %lv.t2
  %t2 = call %struct.s3 () @fr_s3()
  store %struct.s3 %t2, ptr %lv.t3
  %t3 = call %struct.s4 () @fr_s4()
  store %struct.s4 %t3, ptr %lv.t4
  %t4 = call %struct.s5 () @fr_s5()
  store %struct.s5 %t4, ptr %lv.t5
  %t5 = call %struct.s6 () @fr_s6()
  store %struct.s6 %t5, ptr %lv.t6
  %t6 = call %struct.s7 () @fr_s7()
  store %struct.s7 %t6, ptr %lv.t7
  %t7 = call %struct.s8 () @fr_s8()
  store %struct.s8 %t7, ptr %lv.t8
  %t8 = call %struct.s9 () @fr_s9()
  store %struct.s9 %t8, ptr %lv.t9
  %t9 = call %struct.s10 () @fr_s10()
  store %struct.s10 %t9, ptr %lv.t10
  %t10 = call %struct.s11 () @fr_s11()
  store %struct.s11 %t10, ptr %lv.t11
  %t11 = call %struct.s12 () @fr_s12()
  store %struct.s12 %t11, ptr %lv.t12
  %t12 = call %struct.s13 () @fr_s13()
  store %struct.s13 %t12, ptr %lv.t13
  %t13 = call %struct.s14 () @fr_s14()
  store %struct.s14 %t13, ptr %lv.t14
  %t14 = call %struct.s15 () @fr_s15()
  store %struct.s15 %t14, ptr %lv.t15
  %t15 = call %struct.s16 () @fr_s16()
  store %struct.s16 %t15, ptr %lv.t16
  %t16 = call %struct.s17 () @fr_s17()
  store %struct.s17 %t16, ptr %lv.t17
  %t17 = getelementptr [16 x i8], ptr @.str29, i64 0, i64 0
  %t18 = call i32 (ptr, ...) @printf(ptr %t17)
  %t19 = getelementptr [6 x i8], ptr @.str0, i64 0, i64 0
  %t20 = getelementptr %struct.s1, ptr %lv.t1, i32 0, i32 0
  %t21 = getelementptr [1 x i8], ptr %t20, i64 0, i64 0
  %t22 = call i32 (ptr, ...) @printf(ptr %t19, ptr %t21)
  %t23 = getelementptr [6 x i8], ptr @.str1, i64 0, i64 0
  %t24 = getelementptr %struct.s2, ptr %lv.t2, i32 0, i32 0
  %t25 = getelementptr [2 x i8], ptr %t24, i64 0, i64 0
  %t26 = call i32 (ptr, ...) @printf(ptr %t23, ptr %t25)
  %t27 = getelementptr [6 x i8], ptr @.str2, i64 0, i64 0
  %t28 = getelementptr %struct.s3, ptr %lv.t3, i32 0, i32 0
  %t29 = getelementptr [3 x i8], ptr %t28, i64 0, i64 0
  %t30 = call i32 (ptr, ...) @printf(ptr %t27, ptr %t29)
  %t31 = getelementptr [6 x i8], ptr @.str3, i64 0, i64 0
  %t32 = getelementptr %struct.s4, ptr %lv.t4, i32 0, i32 0
  %t33 = getelementptr [4 x i8], ptr %t32, i64 0, i64 0
  %t34 = call i32 (ptr, ...) @printf(ptr %t31, ptr %t33)
  %t35 = getelementptr [6 x i8], ptr @.str4, i64 0, i64 0
  %t36 = getelementptr %struct.s5, ptr %lv.t5, i32 0, i32 0
  %t37 = getelementptr [5 x i8], ptr %t36, i64 0, i64 0
  %t38 = call i32 (ptr, ...) @printf(ptr %t35, ptr %t37)
  %t39 = getelementptr [6 x i8], ptr @.str5, i64 0, i64 0
  %t40 = getelementptr %struct.s6, ptr %lv.t6, i32 0, i32 0
  %t41 = getelementptr [6 x i8], ptr %t40, i64 0, i64 0
  %t42 = call i32 (ptr, ...) @printf(ptr %t39, ptr %t41)
  %t43 = getelementptr [6 x i8], ptr @.str6, i64 0, i64 0
  %t44 = getelementptr %struct.s7, ptr %lv.t7, i32 0, i32 0
  %t45 = getelementptr [7 x i8], ptr %t44, i64 0, i64 0
  %t46 = call i32 (ptr, ...) @printf(ptr %t43, ptr %t45)
  %t47 = getelementptr [6 x i8], ptr @.str7, i64 0, i64 0
  %t48 = getelementptr %struct.s8, ptr %lv.t8, i32 0, i32 0
  %t49 = getelementptr [8 x i8], ptr %t48, i64 0, i64 0
  %t50 = call i32 (ptr, ...) @printf(ptr %t47, ptr %t49)
  %t51 = getelementptr [6 x i8], ptr @.str8, i64 0, i64 0
  %t52 = getelementptr %struct.s9, ptr %lv.t9, i32 0, i32 0
  %t53 = getelementptr [9 x i8], ptr %t52, i64 0, i64 0
  %t54 = call i32 (ptr, ...) @printf(ptr %t51, ptr %t53)
  %t55 = getelementptr [7 x i8], ptr @.str9, i64 0, i64 0
  %t56 = getelementptr %struct.s10, ptr %lv.t10, i32 0, i32 0
  %t57 = getelementptr [10 x i8], ptr %t56, i64 0, i64 0
  %t58 = call i32 (ptr, ...) @printf(ptr %t55, ptr %t57)
  %t59 = getelementptr [7 x i8], ptr @.str10, i64 0, i64 0
  %t60 = getelementptr %struct.s11, ptr %lv.t11, i32 0, i32 0
  %t61 = getelementptr [11 x i8], ptr %t60, i64 0, i64 0
  %t62 = call i32 (ptr, ...) @printf(ptr %t59, ptr %t61)
  %t63 = getelementptr [7 x i8], ptr @.str11, i64 0, i64 0
  %t64 = getelementptr %struct.s12, ptr %lv.t12, i32 0, i32 0
  %t65 = getelementptr [12 x i8], ptr %t64, i64 0, i64 0
  %t66 = call i32 (ptr, ...) @printf(ptr %t63, ptr %t65)
  %t67 = getelementptr [7 x i8], ptr @.str12, i64 0, i64 0
  %t68 = getelementptr %struct.s13, ptr %lv.t13, i32 0, i32 0
  %t69 = getelementptr [13 x i8], ptr %t68, i64 0, i64 0
  %t70 = call i32 (ptr, ...) @printf(ptr %t67, ptr %t69)
  %t71 = getelementptr [7 x i8], ptr @.str13, i64 0, i64 0
  %t72 = getelementptr %struct.s14, ptr %lv.t14, i32 0, i32 0
  %t73 = getelementptr [14 x i8], ptr %t72, i64 0, i64 0
  %t74 = call i32 (ptr, ...) @printf(ptr %t71, ptr %t73)
  %t75 = getelementptr [7 x i8], ptr @.str14, i64 0, i64 0
  %t76 = getelementptr %struct.s15, ptr %lv.t15, i32 0, i32 0
  %t77 = getelementptr [15 x i8], ptr %t76, i64 0, i64 0
  %t78 = call i32 (ptr, ...) @printf(ptr %t75, ptr %t77)
  %t79 = getelementptr [7 x i8], ptr @.str15, i64 0, i64 0
  %t80 = getelementptr %struct.s16, ptr %lv.t16, i32 0, i32 0
  %t81 = getelementptr [16 x i8], ptr %t80, i64 0, i64 0
  %t82 = call i32 (ptr, ...) @printf(ptr %t79, ptr %t81)
  %t83 = getelementptr [7 x i8], ptr @.str16, i64 0, i64 0
  %t84 = getelementptr %struct.s17, ptr %lv.t17, i32 0, i32 0
  %t85 = getelementptr [17 x i8], ptr %t84, i64 0, i64 0
  %t86 = call i32 (ptr, ...) @printf(ptr %t83, ptr %t85)
  %t87 = getelementptr [6 x i8], ptr @.str17, i64 0, i64 0
  %t88 = call %struct.hfa11 () @fr_hfa11()
  store %struct.hfa11 %t88, ptr %t89.agg
  %t90 = getelementptr %struct.hfa11, ptr %t89.agg, i32 0, i32 0
  %t91 = load float, ptr %t90
  %t92 = fpext float %t91 to double
  %t93 = call i32 (ptr, ...) @printf(ptr %t87, double %t92)
  %t94 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t95 = call %struct.hfa12 () @fr_hfa12()
  store %struct.hfa12 %t95, ptr %t96.agg
  %t97 = getelementptr %struct.hfa12, ptr %t96.agg, i32 0, i32 0
  %t98 = load float, ptr %t97
  %t99 = fpext float %t98 to double
  %t100 = call %struct.hfa12 () @fr_hfa12()
  store %struct.hfa12 %t100, ptr %t101.agg
  %t102 = getelementptr %struct.hfa12, ptr %t101.agg, i32 0, i32 1
  %t103 = load float, ptr %t102
  %t104 = fpext float %t103 to double
  %t105 = call i32 (ptr, ...) @printf(ptr %t94, double %t99, double %t104)
  %t106 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t107 = call %struct.hfa13 () @fr_hfa13()
  store %struct.hfa13 %t107, ptr %t108.agg
  %t109 = getelementptr %struct.hfa13, ptr %t108.agg, i32 0, i32 0
  %t110 = load float, ptr %t109
  %t111 = fpext float %t110 to double
  %t112 = call %struct.hfa13 () @fr_hfa13()
  store %struct.hfa13 %t112, ptr %t113.agg
  %t114 = getelementptr %struct.hfa13, ptr %t113.agg, i32 0, i32 2
  %t115 = load float, ptr %t114
  %t116 = fpext float %t115 to double
  %t117 = call i32 (ptr, ...) @printf(ptr %t106, double %t111, double %t116)
  %t118 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t119 = call %struct.hfa14 () @fr_hfa14()
  store %struct.hfa14 %t119, ptr %t120.agg
  %t121 = getelementptr %struct.hfa14, ptr %t120.agg, i32 0, i32 0
  %t122 = load float, ptr %t121
  %t123 = fpext float %t122 to double
  %t124 = call %struct.hfa14 () @fr_hfa14()
  store %struct.hfa14 %t124, ptr %t125.agg
  %t126 = getelementptr %struct.hfa14, ptr %t125.agg, i32 0, i32 3
  %t127 = load float, ptr %t126
  %t128 = fpext float %t127 to double
  %t129 = call i32 (ptr, ...) @printf(ptr %t118, double %t123, double %t128)
  %t130 = getelementptr [6 x i8], ptr @.str17, i64 0, i64 0
  %t131 = call %struct.hfa21 () @fr_hfa21()
  store %struct.hfa21 %t131, ptr %t132.agg
  %t133 = getelementptr %struct.hfa21, ptr %t132.agg, i32 0, i32 0
  %t134 = load double, ptr %t133
  %t135 = call i32 (ptr, ...) @printf(ptr %t130, double %t134)
  %t136 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t137 = call %struct.hfa22 () @fr_hfa22()
  store %struct.hfa22 %t137, ptr %t138.agg
  %t139 = getelementptr %struct.hfa22, ptr %t138.agg, i32 0, i32 0
  %t140 = load double, ptr %t139
  %t141 = call %struct.hfa22 () @fr_hfa22()
  store %struct.hfa22 %t141, ptr %t142.agg
  %t143 = getelementptr %struct.hfa22, ptr %t142.agg, i32 0, i32 1
  %t144 = load double, ptr %t143
  %t145 = call i32 (ptr, ...) @printf(ptr %t136, double %t140, double %t144)
  %t146 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t147 = call %struct.hfa23 () @fr_hfa23()
  store %struct.hfa23 %t147, ptr %t148.agg
  %t149 = getelementptr %struct.hfa23, ptr %t148.agg, i32 0, i32 0
  %t150 = load double, ptr %t149
  %t151 = call %struct.hfa23 () @fr_hfa23()
  store %struct.hfa23 %t151, ptr %t152.agg
  %t153 = getelementptr %struct.hfa23, ptr %t152.agg, i32 0, i32 2
  %t154 = load double, ptr %t153
  %t155 = call i32 (ptr, ...) @printf(ptr %t146, double %t150, double %t154)
  %t156 = getelementptr [11 x i8], ptr @.str18, i64 0, i64 0
  %t157 = call %struct.hfa24 () @fr_hfa24()
  store %struct.hfa24 %t157, ptr %t158.agg
  %t159 = getelementptr %struct.hfa24, ptr %t158.agg, i32 0, i32 0
  %t160 = load double, ptr %t159
  %t161 = call %struct.hfa24 () @fr_hfa24()
  store %struct.hfa24 %t161, ptr %t162.agg
  %t163 = getelementptr %struct.hfa24, ptr %t162.agg, i32 0, i32 3
  %t164 = load double, ptr %t163
  %t165 = call i32 (ptr, ...) @printf(ptr %t156, double %t160, double %t164)
  %t166 = getelementptr [7 x i8], ptr @.str21, i64 0, i64 0
  %t167 = call %struct.hfa31 () @fr_hfa31()
  store %struct.hfa31 %t167, ptr %t168.agg
  %t169 = getelementptr %struct.hfa31, ptr %t168.agg, i32 0, i32 0
  %t170 = load fp128, ptr %t169
  %t171 = call i32 (ptr, ...) @printf(ptr %t166, fp128 %t170)
  %t172 = getelementptr [13 x i8], ptr @.str22, i64 0, i64 0
  %t173 = call %struct.hfa32 () @fr_hfa32()
  store %struct.hfa32 %t173, ptr %t174.agg
  %t175 = getelementptr %struct.hfa32, ptr %t174.agg, i32 0, i32 0
  %t176 = load fp128, ptr %t175
  %t177 = call %struct.hfa32 () @fr_hfa32()
  store %struct.hfa32 %t177, ptr %t178.agg
  %t179 = getelementptr %struct.hfa32, ptr %t178.agg, i32 0, i32 1
  %t180 = load fp128, ptr %t179
  %t181 = call i32 (ptr, ...) @printf(ptr %t172, fp128 %t176, fp128 %t180)
  %t182 = getelementptr [13 x i8], ptr @.str22, i64 0, i64 0
  %t183 = call %struct.hfa33 () @fr_hfa33()
  store %struct.hfa33 %t183, ptr %t184.agg
  %t185 = getelementptr %struct.hfa33, ptr %t184.agg, i32 0, i32 0
  %t186 = load fp128, ptr %t185
  %t187 = call %struct.hfa33 () @fr_hfa33()
  store %struct.hfa33 %t187, ptr %t188.agg
  %t189 = getelementptr %struct.hfa33, ptr %t188.agg, i32 0, i32 2
  %t190 = load fp128, ptr %t189
  %t191 = call i32 (ptr, ...) @printf(ptr %t182, fp128 %t186, fp128 %t190)
  %t192 = getelementptr [13 x i8], ptr @.str22, i64 0, i64 0
  %t193 = call %struct.hfa34 () @fr_hfa34()
  store %struct.hfa34 %t193, ptr %t194.agg
  %t195 = getelementptr %struct.hfa34, ptr %t194.agg, i32 0, i32 0
  %t196 = load fp128, ptr %t195
  %t197 = call %struct.hfa34 () @fr_hfa34()
  store %struct.hfa34 %t197, ptr %t198.agg
  %t199 = getelementptr %struct.hfa34, ptr %t198.agg, i32 0, i32 3
  %t200 = load fp128, ptr %t199
  %t201 = call i32 (ptr, ...) @printf(ptr %t192, fp128 %t196, fp128 %t200)
  ret void
}

define i32 @match(ptr %p.s, ptr %p.f)
{
entry:
  %lv.param.f = alloca ptr, align 8
  store ptr %p.f, ptr %lv.param.f
  %lv.p = alloca ptr, align 8
  %t0 = load ptr, ptr %p.s
  store ptr %t0, ptr %lv.p
  %t1 = load ptr, ptr %p.s
  store ptr %t1, ptr %lv.p
  br label %for.cond.65
for.cond.65:
  %t2 = load ptr, ptr %lv.param.f
  %t3 = load i8, ptr %t2
  %t4 = icmp ne i8 %t3, 0
  br i1 %t4, label %logic.rhs.5, label %logic.skip.6
logic.rhs.5:
  %t9 = load ptr, ptr %lv.param.f
  %t10 = load i8, ptr %t9
  %t11 = load ptr, ptr %lv.p
  %t12 = load i8, ptr %t11
  %t13 = sext i8 %t10 to i32
  %t14 = sext i8 %t12 to i32
  %t15 = icmp eq i32 %t13, %t14
  %t16 = zext i1 %t15 to i32
  %t17 = icmp ne i32 %t16, 0
  %t18 = zext i1 %t17 to i32
  br label %logic.rhs.end.7
logic.rhs.end.7:
  br label %logic.end.8
logic.skip.6:
  br label %logic.end.8
logic.end.8:
  %t19 = phi i32 [ %t18, %logic.rhs.end.7 ], [ 0, %logic.skip.6 ]
  %t20 = icmp ne i32 %t19, 0
  br i1 %t20, label %block_65, label %block_66
for.latch.65:
  %t21 = load ptr, ptr %lv.param.f
  %t22 = getelementptr i8, ptr %t21, i64 1
  store ptr %t22, ptr %lv.param.f
  %t23 = load ptr, ptr %lv.p
  %t24 = getelementptr i8, ptr %t23, i64 1
  store ptr %t24, ptr %lv.p
  br label %for.cond.65
block_65:
  br label %for.latch.65
block_66:
  %t25 = load ptr, ptr %lv.param.f
  %t26 = load i8, ptr %t25
  %t27 = icmp ne i8 %t26, 0
  %t28 = xor i1 %t27, true
  %t29 = zext i1 %t28 to i32
  %t30 = icmp ne i32 %t29, 0
  br i1 %t30, label %block_67, label %block_68
block_67:
  %t31 = load ptr, ptr %lv.p
  %t32 = sext i32 1 to i64
  %t33 = sub i64 0, %t32
  %t34 = getelementptr i8, ptr %t31, i64 %t33
  store ptr %t34, ptr %p.s
  ret i32 1
block_68:
  ret i32 0
}

define void @myprintf(ptr %p.format, ...)
{
entry:
  %lv.s = alloca ptr, align 8
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  %lv.t7 = alloca %struct.s7, align 1
  store %struct.s7 zeroinitializer, ptr %lv.t7
  %lv.t9 = alloca %struct.s9, align 1
  store %struct.s9 zeroinitializer, ptr %lv.t9
  %lv.x = alloca %struct.hfa11, align 4
  store %struct.hfa11 zeroinitializer, ptr %lv.x
  %lv.x.1 = alloca %struct.hfa12, align 4
  store %struct.hfa12 zeroinitializer, ptr %lv.x.1
  %lv.x.2 = alloca %struct.hfa13, align 4
  store %struct.hfa13 zeroinitializer, ptr %lv.x.2
  %lv.x.3 = alloca %struct.hfa14, align 4
  store %struct.hfa14 zeroinitializer, ptr %lv.x.3
  %lv.x.4 = alloca %struct.hfa21, align 8
  store %struct.hfa21 zeroinitializer, ptr %lv.x.4
  %lv.x.5 = alloca %struct.hfa22, align 8
  store %struct.hfa22 zeroinitializer, ptr %lv.x.5
  %lv.x.6 = alloca %struct.hfa23, align 8
  store %struct.hfa23 zeroinitializer, ptr %lv.x.6
  %lv.x.7 = alloca %struct.hfa24, align 8
  store %struct.hfa24 zeroinitializer, ptr %lv.x.7
  %lv.x.8 = alloca %struct.hfa31, align 16
  store %struct.hfa31 zeroinitializer, ptr %lv.x.8
  %lv.x.9 = alloca %struct.hfa32, align 16
  store %struct.hfa32 zeroinitializer, ptr %lv.x.9
  %lv.x.10 = alloca %struct.hfa33, align 16
  store %struct.hfa33 zeroinitializer, ptr %lv.x.10
  %lv.x.11 = alloca %struct.hfa34, align 16
  store %struct.hfa34 zeroinitializer, ptr %lv.x.11
  call void @llvm.va_start.p0(ptr %lv.ap)
  store ptr %p.format, ptr %lv.s
  br label %for.cond.70
for.cond.70:
  %t0 = load ptr, ptr %lv.s
  %t1 = load i8, ptr %t0
  %t2 = icmp ne i8 %t1, 0
  br i1 %t2, label %block_70, label %block_71
for.latch.70:
  %t3 = load ptr, ptr %lv.s
  %t4 = getelementptr i8, ptr %t3, i64 1
  store ptr %t4, ptr %lv.s
  br label %for.cond.70
block_70:
  %t5 = getelementptr [4 x i8], ptr @.str30, i64 0, i64 0
  %t6 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t5)
  %t7 = icmp ne i32 %t6, 0
  br i1 %t7, label %block_72, label %block_73
block_71:
  %t8 = call i32 (i32) @putchar(i32 10)
  ret void
block_72:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t10 = load i32, ptr %t9
  %t15 = icmp sge i32 %t10, 0
  br i1 %t15, label %vaarg.stack.11, label %vaarg.regtry.12
vaarg.regtry.12:
  %t16 = add i32 %t10, 8
  store i32 %t16, ptr %t9
  %t17 = icmp sle i32 %t16, 0
  br i1 %t17, label %vaarg.reg.13, label %vaarg.stack.11
vaarg.reg.13:
  %t18 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t19 = load ptr, ptr %t18
  %t20 = getelementptr i8, ptr %t19, i32 %t10
  br label %vaarg.join.14
vaarg.stack.11:
  %t21 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t22 = load ptr, ptr %t21
  %t23 = getelementptr i8, ptr %t22, i64 8
  store ptr %t23, ptr %t21
  br label %vaarg.join.14
vaarg.join.14:
  %t24 = phi ptr [ %t20, %vaarg.reg.13 ], [ %t22, %vaarg.stack.11 ]
  %t25 = alloca %struct.s7
  call void @llvm.memcpy.p0.p0.i64(ptr %t25, ptr %t24, i64 7, i1 false)
  %t26 = load %struct.s7, ptr %t25
  store %struct.s7 %t26, ptr %lv.t7
  %t27 = getelementptr [5 x i8], ptr @.str31, i64 0, i64 0
  %t28 = getelementptr %struct.s7, ptr %lv.t7, i32 0, i32 0
  %t29 = getelementptr [7 x i8], ptr %t28, i64 0, i64 0
  %t30 = call i32 (ptr, ...) @printf(ptr %t27, ptr %t29)
  br label %block_74
block_73:
  %t31 = getelementptr [4 x i8], ptr @.str32, i64 0, i64 0
  %t32 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t31)
  %t33 = icmp ne i32 %t32, 0
  br i1 %t33, label %block_75, label %block_76
block_74:
  br label %for.latch.70
block_75:
  %t34 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t35 = load i32, ptr %t34
  %t40 = icmp sge i32 %t35, 0
  br i1 %t40, label %vaarg.stack.36, label %vaarg.regtry.37
vaarg.regtry.37:
  %t41 = add i32 %t35, 16
  store i32 %t41, ptr %t34
  %t42 = icmp sle i32 %t41, 0
  br i1 %t42, label %vaarg.reg.38, label %vaarg.stack.36
vaarg.reg.38:
  %t43 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t44 = load ptr, ptr %t43
  %t45 = getelementptr i8, ptr %t44, i32 %t35
  br label %vaarg.join.39
vaarg.stack.36:
  %t46 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t47 = load ptr, ptr %t46
  %t48 = getelementptr i8, ptr %t47, i64 16
  store ptr %t48, ptr %t46
  br label %vaarg.join.39
vaarg.join.39:
  %t49 = phi ptr [ %t45, %vaarg.reg.38 ], [ %t47, %vaarg.stack.36 ]
  %t50 = alloca %struct.s9
  call void @llvm.memcpy.p0.p0.i64(ptr %t50, ptr %t49, i64 9, i1 false)
  %t51 = load %struct.s9, ptr %t50
  store %struct.s9 %t51, ptr %lv.t9
  %t52 = getelementptr [5 x i8], ptr @.str33, i64 0, i64 0
  %t53 = getelementptr %struct.s9, ptr %lv.t9, i32 0, i32 0
  %t54 = getelementptr [9 x i8], ptr %t53, i64 0, i64 0
  %t55 = call i32 (ptr, ...) @printf(ptr %t52, ptr %t54)
  br label %block_77
block_76:
  %t56 = getelementptr [7 x i8], ptr @.str34, i64 0, i64 0
  %t57 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t56)
  %t58 = icmp ne i32 %t57, 0
  br i1 %t58, label %block_78, label %block_79
block_77:
  br label %block_74
block_78:
  %t59 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t60 = load i32, ptr %t59
  %t65 = icmp sge i32 %t60, 0
  br i1 %t65, label %vaarg.stack.61, label %vaarg.regtry.62
vaarg.regtry.62:
  %t66 = add i32 %t60, 8
  store i32 %t66, ptr %t59
  %t67 = icmp sle i32 %t66, 0
  br i1 %t67, label %vaarg.reg.63, label %vaarg.stack.61
vaarg.reg.63:
  %t68 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t69 = load ptr, ptr %t68
  %t70 = getelementptr i8, ptr %t69, i32 %t60
  br label %vaarg.join.64
vaarg.stack.61:
  %t71 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t72 = load ptr, ptr %t71
  %t73 = getelementptr i8, ptr %t72, i64 8
  store ptr %t73, ptr %t71
  br label %vaarg.join.64
vaarg.join.64:
  %t74 = phi ptr [ %t70, %vaarg.reg.63 ], [ %t72, %vaarg.stack.61 ]
  %t75 = alloca %struct.hfa11
  call void @llvm.memcpy.p0.p0.i64(ptr %t75, ptr %t74, i64 4, i1 false)
  %t76 = load %struct.hfa11, ptr %t75
  store %struct.hfa11 %t76, ptr %lv.x
  %t77 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t78 = getelementptr %struct.hfa11, ptr %lv.x, i32 0, i32 0
  %t79 = load float, ptr %t78
  %t80 = fpext float %t79 to double
  %t81 = getelementptr %struct.hfa11, ptr %lv.x, i32 0, i32 0
  %t82 = load float, ptr %t81
  %t83 = fpext float %t82 to double
  %t84 = call i32 (ptr, ...) @printf(ptr %t77, double %t80, double %t83)
  br label %block_80
block_79:
  %t85 = getelementptr [7 x i8], ptr @.str36, i64 0, i64 0
  %t86 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t85)
  %t87 = icmp ne i32 %t86, 0
  br i1 %t87, label %block_81, label %block_82
block_80:
  br label %block_77
block_81:
  %t88 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t89 = load i32, ptr %t88
  %t94 = icmp sge i32 %t89, 0
  br i1 %t94, label %vaarg.stack.90, label %vaarg.regtry.91
vaarg.regtry.91:
  %t95 = add i32 %t89, 8
  store i32 %t95, ptr %t88
  %t96 = icmp sle i32 %t95, 0
  br i1 %t96, label %vaarg.reg.92, label %vaarg.stack.90
vaarg.reg.92:
  %t97 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t98 = load ptr, ptr %t97
  %t99 = getelementptr i8, ptr %t98, i32 %t89
  br label %vaarg.join.93
vaarg.stack.90:
  %t100 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t101 = load ptr, ptr %t100
  %t102 = getelementptr i8, ptr %t101, i64 8
  store ptr %t102, ptr %t100
  br label %vaarg.join.93
vaarg.join.93:
  %t103 = phi ptr [ %t99, %vaarg.reg.92 ], [ %t101, %vaarg.stack.90 ]
  %t104 = alloca %struct.hfa12
  call void @llvm.memcpy.p0.p0.i64(ptr %t104, ptr %t103, i64 8, i1 false)
  %t105 = load %struct.hfa12, ptr %t104
  store %struct.hfa12 %t105, ptr %lv.x.1
  %t106 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t107 = getelementptr %struct.hfa12, ptr %lv.x.1, i32 0, i32 0
  %t108 = load float, ptr %t107
  %t109 = fpext float %t108 to double
  %t110 = getelementptr %struct.hfa12, ptr %lv.x.1, i32 0, i32 1
  %t111 = load float, ptr %t110
  %t112 = fpext float %t111 to double
  %t113 = call i32 (ptr, ...) @printf(ptr %t106, double %t109, double %t112)
  br label %block_83
block_82:
  %t114 = getelementptr [7 x i8], ptr @.str37, i64 0, i64 0
  %t115 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t114)
  %t116 = icmp ne i32 %t115, 0
  br i1 %t116, label %block_84, label %block_85
block_83:
  br label %block_80
block_84:
  %t117 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t118 = load i32, ptr %t117
  %t123 = icmp sge i32 %t118, 0
  br i1 %t123, label %vaarg.stack.119, label %vaarg.regtry.120
vaarg.regtry.120:
  %t124 = add i32 %t118, 16
  store i32 %t124, ptr %t117
  %t125 = icmp sle i32 %t124, 0
  br i1 %t125, label %vaarg.reg.121, label %vaarg.stack.119
vaarg.reg.121:
  %t126 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t127 = load ptr, ptr %t126
  %t128 = getelementptr i8, ptr %t127, i32 %t118
  br label %vaarg.join.122
vaarg.stack.119:
  %t129 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t130 = load ptr, ptr %t129
  %t131 = getelementptr i8, ptr %t130, i64 16
  store ptr %t131, ptr %t129
  br label %vaarg.join.122
vaarg.join.122:
  %t132 = phi ptr [ %t128, %vaarg.reg.121 ], [ %t130, %vaarg.stack.119 ]
  %t133 = alloca %struct.hfa13
  call void @llvm.memcpy.p0.p0.i64(ptr %t133, ptr %t132, i64 12, i1 false)
  %t134 = load %struct.hfa13, ptr %t133
  store %struct.hfa13 %t134, ptr %lv.x.2
  %t135 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t136 = getelementptr %struct.hfa13, ptr %lv.x.2, i32 0, i32 0
  %t137 = load float, ptr %t136
  %t138 = fpext float %t137 to double
  %t139 = getelementptr %struct.hfa13, ptr %lv.x.2, i32 0, i32 2
  %t140 = load float, ptr %t139
  %t141 = fpext float %t140 to double
  %t142 = call i32 (ptr, ...) @printf(ptr %t135, double %t138, double %t141)
  br label %block_86
block_85:
  %t143 = getelementptr [7 x i8], ptr @.str38, i64 0, i64 0
  %t144 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t143)
  %t145 = icmp ne i32 %t144, 0
  br i1 %t145, label %block_87, label %block_88
block_86:
  br label %block_83
block_87:
  %t146 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t147 = load i32, ptr %t146
  %t152 = icmp sge i32 %t147, 0
  br i1 %t152, label %vaarg.stack.148, label %vaarg.regtry.149
vaarg.regtry.149:
  %t153 = add i32 %t147, 16
  store i32 %t153, ptr %t146
  %t154 = icmp sle i32 %t153, 0
  br i1 %t154, label %vaarg.reg.150, label %vaarg.stack.148
vaarg.reg.150:
  %t155 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t156 = load ptr, ptr %t155
  %t157 = getelementptr i8, ptr %t156, i32 %t147
  br label %vaarg.join.151
vaarg.stack.148:
  %t158 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t159 = load ptr, ptr %t158
  %t160 = getelementptr i8, ptr %t159, i64 16
  store ptr %t160, ptr %t158
  br label %vaarg.join.151
vaarg.join.151:
  %t161 = phi ptr [ %t157, %vaarg.reg.150 ], [ %t159, %vaarg.stack.148 ]
  %t162 = alloca %struct.hfa14
  call void @llvm.memcpy.p0.p0.i64(ptr %t162, ptr %t161, i64 16, i1 false)
  %t163 = load %struct.hfa14, ptr %t162
  store %struct.hfa14 %t163, ptr %lv.x.3
  %t164 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t165 = getelementptr %struct.hfa14, ptr %lv.x.3, i32 0, i32 0
  %t166 = load float, ptr %t165
  %t167 = fpext float %t166 to double
  %t168 = getelementptr %struct.hfa14, ptr %lv.x.3, i32 0, i32 3
  %t169 = load float, ptr %t168
  %t170 = fpext float %t169 to double
  %t171 = call i32 (ptr, ...) @printf(ptr %t164, double %t167, double %t170)
  br label %block_89
block_88:
  %t172 = getelementptr [7 x i8], ptr @.str39, i64 0, i64 0
  %t173 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t172)
  %t174 = icmp ne i32 %t173, 0
  br i1 %t174, label %block_90, label %block_91
block_89:
  br label %block_86
block_90:
  %t175 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t176 = load i32, ptr %t175
  %t181 = icmp sge i32 %t176, 0
  br i1 %t181, label %vaarg.stack.177, label %vaarg.regtry.178
vaarg.regtry.178:
  %t182 = add i32 %t176, 8
  store i32 %t182, ptr %t175
  %t183 = icmp sle i32 %t182, 0
  br i1 %t183, label %vaarg.reg.179, label %vaarg.stack.177
vaarg.reg.179:
  %t184 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t185 = load ptr, ptr %t184
  %t186 = getelementptr i8, ptr %t185, i32 %t176
  br label %vaarg.join.180
vaarg.stack.177:
  %t187 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t188 = load ptr, ptr %t187
  %t189 = getelementptr i8, ptr %t188, i64 8
  store ptr %t189, ptr %t187
  br label %vaarg.join.180
vaarg.join.180:
  %t190 = phi ptr [ %t186, %vaarg.reg.179 ], [ %t188, %vaarg.stack.177 ]
  %t191 = alloca %struct.hfa21
  call void @llvm.memcpy.p0.p0.i64(ptr %t191, ptr %t190, i64 8, i1 false)
  %t192 = load %struct.hfa21, ptr %t191
  store %struct.hfa21 %t192, ptr %lv.x.4
  %t193 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t194 = getelementptr %struct.hfa21, ptr %lv.x.4, i32 0, i32 0
  %t195 = load double, ptr %t194
  %t196 = getelementptr %struct.hfa21, ptr %lv.x.4, i32 0, i32 0
  %t197 = load double, ptr %t196
  %t198 = call i32 (ptr, ...) @printf(ptr %t193, double %t195, double %t197)
  br label %block_92
block_91:
  %t199 = getelementptr [7 x i8], ptr @.str40, i64 0, i64 0
  %t200 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t199)
  %t201 = icmp ne i32 %t200, 0
  br i1 %t201, label %block_93, label %block_94
block_92:
  br label %block_89
block_93:
  %t202 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t203 = load i32, ptr %t202
  %t208 = icmp sge i32 %t203, 0
  br i1 %t208, label %vaarg.stack.204, label %vaarg.regtry.205
vaarg.regtry.205:
  %t209 = add i32 %t203, 16
  store i32 %t209, ptr %t202
  %t210 = icmp sle i32 %t209, 0
  br i1 %t210, label %vaarg.reg.206, label %vaarg.stack.204
vaarg.reg.206:
  %t211 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t212 = load ptr, ptr %t211
  %t213 = getelementptr i8, ptr %t212, i32 %t203
  br label %vaarg.join.207
vaarg.stack.204:
  %t214 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t215 = load ptr, ptr %t214
  %t216 = getelementptr i8, ptr %t215, i64 16
  store ptr %t216, ptr %t214
  br label %vaarg.join.207
vaarg.join.207:
  %t217 = phi ptr [ %t213, %vaarg.reg.206 ], [ %t215, %vaarg.stack.204 ]
  %t218 = alloca %struct.hfa22
  call void @llvm.memcpy.p0.p0.i64(ptr %t218, ptr %t217, i64 16, i1 false)
  %t219 = load %struct.hfa22, ptr %t218
  store %struct.hfa22 %t219, ptr %lv.x.5
  %t220 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t221 = getelementptr %struct.hfa22, ptr %lv.x.5, i32 0, i32 0
  %t222 = load double, ptr %t221
  %t223 = getelementptr %struct.hfa22, ptr %lv.x.5, i32 0, i32 1
  %t224 = load double, ptr %t223
  %t225 = call i32 (ptr, ...) @printf(ptr %t220, double %t222, double %t224)
  br label %block_95
block_94:
  %t226 = getelementptr [7 x i8], ptr @.str41, i64 0, i64 0
  %t227 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t226)
  %t228 = icmp ne i32 %t227, 0
  br i1 %t228, label %block_96, label %block_97
block_95:
  br label %block_92
block_96:
  %t229 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t230 = load i32, ptr %t229
  %t235 = icmp sge i32 %t230, 0
  br i1 %t235, label %vaarg.stack.231, label %vaarg.regtry.232
vaarg.regtry.232:
  %t236 = add i32 %t230, 8
  store i32 %t236, ptr %t229
  %t237 = icmp sle i32 %t236, 0
  br i1 %t237, label %vaarg.reg.233, label %vaarg.stack.231
vaarg.reg.233:
  %t238 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t239 = load ptr, ptr %t238
  %t240 = getelementptr i8, ptr %t239, i32 %t230
  br label %vaarg.join.234
vaarg.stack.231:
  %t241 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t242 = load ptr, ptr %t241
  %t243 = getelementptr i8, ptr %t242, i64 8
  store ptr %t243, ptr %t241
  br label %vaarg.join.234
vaarg.join.234:
  %t244 = phi ptr [ %t240, %vaarg.reg.233 ], [ %t242, %vaarg.stack.231 ]
  %t245 = load ptr, ptr %t244
  %t246 = alloca %struct.hfa23
  call void @llvm.memcpy.p0.p0.i64(ptr %t246, ptr %t245, i64 24, i1 false)
  %t247 = load %struct.hfa23, ptr %t246
  store %struct.hfa23 %t247, ptr %lv.x.6
  %t248 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t249 = getelementptr %struct.hfa23, ptr %lv.x.6, i32 0, i32 0
  %t250 = load double, ptr %t249
  %t251 = getelementptr %struct.hfa23, ptr %lv.x.6, i32 0, i32 2
  %t252 = load double, ptr %t251
  %t253 = call i32 (ptr, ...) @printf(ptr %t248, double %t250, double %t252)
  br label %block_98
block_97:
  %t254 = getelementptr [7 x i8], ptr @.str42, i64 0, i64 0
  %t255 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t254)
  %t256 = icmp ne i32 %t255, 0
  br i1 %t256, label %block_99, label %block_100
block_98:
  br label %block_95
block_99:
  %t257 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t258 = load i32, ptr %t257
  %t263 = icmp sge i32 %t258, 0
  br i1 %t263, label %vaarg.stack.259, label %vaarg.regtry.260
vaarg.regtry.260:
  %t264 = add i32 %t258, 8
  store i32 %t264, ptr %t257
  %t265 = icmp sle i32 %t264, 0
  br i1 %t265, label %vaarg.reg.261, label %vaarg.stack.259
vaarg.reg.261:
  %t266 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t267 = load ptr, ptr %t266
  %t268 = getelementptr i8, ptr %t267, i32 %t258
  br label %vaarg.join.262
vaarg.stack.259:
  %t269 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t270 = load ptr, ptr %t269
  %t271 = getelementptr i8, ptr %t270, i64 8
  store ptr %t271, ptr %t269
  br label %vaarg.join.262
vaarg.join.262:
  %t272 = phi ptr [ %t268, %vaarg.reg.261 ], [ %t270, %vaarg.stack.259 ]
  %t273 = load ptr, ptr %t272
  %t274 = alloca %struct.hfa24
  call void @llvm.memcpy.p0.p0.i64(ptr %t274, ptr %t273, i64 32, i1 false)
  %t275 = load %struct.hfa24, ptr %t274
  store %struct.hfa24 %t275, ptr %lv.x.7
  %t276 = getelementptr [10 x i8], ptr @.str35, i64 0, i64 0
  %t277 = getelementptr %struct.hfa24, ptr %lv.x.7, i32 0, i32 0
  %t278 = load double, ptr %t277
  %t279 = getelementptr %struct.hfa24, ptr %lv.x.7, i32 0, i32 3
  %t280 = load double, ptr %t279
  %t281 = call i32 (ptr, ...) @printf(ptr %t276, double %t278, double %t280)
  br label %block_101
block_100:
  %t282 = getelementptr [7 x i8], ptr @.str43, i64 0, i64 0
  %t283 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t282)
  %t284 = icmp ne i32 %t283, 0
  br i1 %t284, label %block_102, label %block_103
block_101:
  br label %block_98
block_102:
  %t285 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t286 = load i32, ptr %t285
  %t291 = icmp sge i32 %t286, 0
  br i1 %t291, label %vaarg.stack.287, label %vaarg.regtry.288
vaarg.regtry.288:
  %t292 = add i32 %t286, 16
  store i32 %t292, ptr %t285
  %t293 = icmp sle i32 %t292, 0
  br i1 %t293, label %vaarg.reg.289, label %vaarg.stack.287
vaarg.reg.289:
  %t294 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t295 = load ptr, ptr %t294
  %t296 = getelementptr i8, ptr %t295, i32 %t286
  br label %vaarg.join.290
vaarg.stack.287:
  %t297 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t298 = load ptr, ptr %t297
  %t299 = getelementptr i8, ptr %t298, i64 16
  store ptr %t299, ptr %t297
  br label %vaarg.join.290
vaarg.join.290:
  %t300 = phi ptr [ %t296, %vaarg.reg.289 ], [ %t298, %vaarg.stack.287 ]
  %t301 = alloca %struct.hfa31
  call void @llvm.memcpy.p0.p0.i64(ptr %t301, ptr %t300, i64 16, i1 false)
  %t302 = load %struct.hfa31, ptr %t301
  store %struct.hfa31 %t302, ptr %lv.x.8
  %t303 = getelementptr [12 x i8], ptr @.str44, i64 0, i64 0
  %t304 = getelementptr %struct.hfa31, ptr %lv.x.8, i32 0, i32 0
  %t305 = load fp128, ptr %t304
  %t306 = getelementptr %struct.hfa31, ptr %lv.x.8, i32 0, i32 0
  %t307 = load fp128, ptr %t306
  %t308 = call i32 (ptr, ...) @printf(ptr %t303, fp128 %t305, fp128 %t307)
  br label %block_104
block_103:
  %t309 = getelementptr [7 x i8], ptr @.str45, i64 0, i64 0
  %t310 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t309)
  %t311 = icmp ne i32 %t310, 0
  br i1 %t311, label %block_105, label %block_106
block_104:
  br label %block_101
block_105:
  %t312 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t313 = load i32, ptr %t312
  %t318 = icmp sge i32 %t313, 0
  br i1 %t318, label %vaarg.stack.314, label %vaarg.regtry.315
vaarg.regtry.315:
  %t319 = add i32 %t313, 8
  store i32 %t319, ptr %t312
  %t320 = icmp sle i32 %t319, 0
  br i1 %t320, label %vaarg.reg.316, label %vaarg.stack.314
vaarg.reg.316:
  %t321 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t322 = load ptr, ptr %t321
  %t323 = getelementptr i8, ptr %t322, i32 %t313
  br label %vaarg.join.317
vaarg.stack.314:
  %t324 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t325 = load ptr, ptr %t324
  %t326 = getelementptr i8, ptr %t325, i64 8
  store ptr %t326, ptr %t324
  br label %vaarg.join.317
vaarg.join.317:
  %t327 = phi ptr [ %t323, %vaarg.reg.316 ], [ %t325, %vaarg.stack.314 ]
  %t328 = load ptr, ptr %t327
  %t329 = alloca %struct.hfa32
  call void @llvm.memcpy.p0.p0.i64(ptr %t329, ptr %t328, i64 32, i1 false)
  %t330 = load %struct.hfa32, ptr %t329
  store %struct.hfa32 %t330, ptr %lv.x.9
  %t331 = getelementptr [12 x i8], ptr @.str44, i64 0, i64 0
  %t332 = getelementptr %struct.hfa32, ptr %lv.x.9, i32 0, i32 0
  %t333 = load fp128, ptr %t332
  %t334 = getelementptr %struct.hfa32, ptr %lv.x.9, i32 0, i32 1
  %t335 = load fp128, ptr %t334
  %t336 = call i32 (ptr, ...) @printf(ptr %t331, fp128 %t333, fp128 %t335)
  br label %block_107
block_106:
  %t337 = getelementptr [7 x i8], ptr @.str46, i64 0, i64 0
  %t338 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t337)
  %t339 = icmp ne i32 %t338, 0
  br i1 %t339, label %block_108, label %block_109
block_107:
  br label %block_104
block_108:
  %t340 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t341 = load i32, ptr %t340
  %t346 = icmp sge i32 %t341, 0
  br i1 %t346, label %vaarg.stack.342, label %vaarg.regtry.343
vaarg.regtry.343:
  %t347 = add i32 %t341, 8
  store i32 %t347, ptr %t340
  %t348 = icmp sle i32 %t347, 0
  br i1 %t348, label %vaarg.reg.344, label %vaarg.stack.342
vaarg.reg.344:
  %t349 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t350 = load ptr, ptr %t349
  %t351 = getelementptr i8, ptr %t350, i32 %t341
  br label %vaarg.join.345
vaarg.stack.342:
  %t352 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t353 = load ptr, ptr %t352
  %t354 = getelementptr i8, ptr %t353, i64 8
  store ptr %t354, ptr %t352
  br label %vaarg.join.345
vaarg.join.345:
  %t355 = phi ptr [ %t351, %vaarg.reg.344 ], [ %t353, %vaarg.stack.342 ]
  %t356 = load ptr, ptr %t355
  %t357 = alloca %struct.hfa33
  call void @llvm.memcpy.p0.p0.i64(ptr %t357, ptr %t356, i64 48, i1 false)
  %t358 = load %struct.hfa33, ptr %t357
  store %struct.hfa33 %t358, ptr %lv.x.10
  %t359 = getelementptr [12 x i8], ptr @.str44, i64 0, i64 0
  %t360 = getelementptr %struct.hfa33, ptr %lv.x.10, i32 0, i32 0
  %t361 = load fp128, ptr %t360
  %t362 = getelementptr %struct.hfa33, ptr %lv.x.10, i32 0, i32 2
  %t363 = load fp128, ptr %t362
  %t364 = call i32 (ptr, ...) @printf(ptr %t359, fp128 %t361, fp128 %t363)
  br label %block_110
block_109:
  %t365 = getelementptr [7 x i8], ptr @.str47, i64 0, i64 0
  %t366 = call i32 (ptr, ptr) @match(ptr %lv.s, ptr %t365)
  %t367 = icmp ne i32 %t366, 0
  br i1 %t367, label %block_111, label %block_112
block_110:
  br label %block_107
block_111:
  %t368 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t369 = load i32, ptr %t368
  %t374 = icmp sge i32 %t369, 0
  br i1 %t374, label %vaarg.stack.370, label %vaarg.regtry.371
vaarg.regtry.371:
  %t375 = add i32 %t369, 8
  store i32 %t375, ptr %t368
  %t376 = icmp sle i32 %t375, 0
  br i1 %t376, label %vaarg.reg.372, label %vaarg.stack.370
vaarg.reg.372:
  %t377 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t378 = load ptr, ptr %t377
  %t379 = getelementptr i8, ptr %t378, i32 %t369
  br label %vaarg.join.373
vaarg.stack.370:
  %t380 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t381 = load ptr, ptr %t380
  %t382 = getelementptr i8, ptr %t381, i64 8
  store ptr %t382, ptr %t380
  br label %vaarg.join.373
vaarg.join.373:
  %t383 = phi ptr [ %t379, %vaarg.reg.372 ], [ %t381, %vaarg.stack.370 ]
  %t384 = load ptr, ptr %t383
  %t385 = alloca %struct.hfa34
  call void @llvm.memcpy.p0.p0.i64(ptr %t385, ptr %t384, i64 64, i1 false)
  %t386 = load %struct.hfa34, ptr %t385
  store %struct.hfa34 %t386, ptr %lv.x.11
  %t387 = getelementptr [12 x i8], ptr @.str44, i64 0, i64 0
  %t388 = getelementptr %struct.hfa34, ptr %lv.x.11, i32 0, i32 0
  %t389 = load fp128, ptr %t388
  %t390 = getelementptr %struct.hfa34, ptr %lv.x.11, i32 0, i32 3
  %t391 = load fp128, ptr %t390
  %t392 = call i32 (ptr, ...) @printf(ptr %t387, fp128 %t389, fp128 %t391)
  br label %block_113
block_112:
  %t393 = load ptr, ptr %lv.s
  %t394 = load i8, ptr %t393
  %t395 = sext i8 %t394 to i32
  %t396 = call i32 (i32) @putchar(i32 %t395)
  br label %block_113
block_113:
  br label %block_110
}

define void @stdarg()
{
entry:
  %t0 = getelementptr [9 x i8], ptr @.str48, i64 0, i64 0
  %t1 = call i32 (ptr, ...) @printf(ptr %t0)
  %t2 = getelementptr [24 x i8], ptr @.str49, i64 0, i64 0
  %t3 = load %struct.s9, ptr @s9
  %t4 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t4, ptr @s9, i64 9, i1 false)
  %t5 = load [2 x i64], ptr %t4
  %t6 = load %struct.s9, ptr @s9
  %t7 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t7, ptr @s9, i64 9, i1 false)
  %t8 = load [2 x i64], ptr %t7
  %t9 = load %struct.s9, ptr @s9
  %t10 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t10, ptr @s9, i64 9, i1 false)
  %t11 = load [2 x i64], ptr %t10
  %t12 = load %struct.s9, ptr @s9
  %t13 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t13, ptr @s9, i64 9, i1 false)
  %t14 = load [2 x i64], ptr %t13
  %t15 = load %struct.s9, ptr @s9
  %t16 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t16, ptr @s9, i64 9, i1 false)
  %t17 = load [2 x i64], ptr %t16
  %t18 = load %struct.s9, ptr @s9
  %t19 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t19, ptr @s9, i64 9, i1 false)
  %t20 = load [2 x i64], ptr %t19
  call void (ptr, ...) @myprintf(ptr %t2, [2 x i64] %t5, [2 x i64] %t8, [2 x i64] %t11, [2 x i64] %t14, [2 x i64] %t17, [2 x i64] %t20)
  %t21 = getelementptr [24 x i8], ptr @.str50, i64 0, i64 0
  %t22 = load %struct.s7, ptr @s7
  %t23 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t23, ptr @s7, i64 7, i1 false)
  %t24 = load i64, ptr %t23
  %t25 = load %struct.s9, ptr @s9
  %t26 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t26, ptr @s9, i64 9, i1 false)
  %t27 = load [2 x i64], ptr %t26
  %t28 = load %struct.s9, ptr @s9
  %t29 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t29, ptr @s9, i64 9, i1 false)
  %t30 = load [2 x i64], ptr %t29
  %t31 = load %struct.s9, ptr @s9
  %t32 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t32, ptr @s9, i64 9, i1 false)
  %t33 = load [2 x i64], ptr %t32
  %t34 = load %struct.s9, ptr @s9
  %t35 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t35, ptr @s9, i64 9, i1 false)
  %t36 = load [2 x i64], ptr %t35
  %t37 = load %struct.s9, ptr @s9
  %t38 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t38, ptr @s9, i64 9, i1 false)
  %t39 = load [2 x i64], ptr %t38
  call void (ptr, ...) @myprintf(ptr %t21, i64 %t24, [2 x i64] %t27, [2 x i64] %t30, [2 x i64] %t33, [2 x i64] %t36, [2 x i64] %t39)
  %t40 = getelementptr [17 x i8], ptr @.str51, i64 0, i64 0
  call void (ptr, ...) @myprintf(ptr %t40)
  %t41 = getelementptr [28 x i8], ptr @.str52, i64 0, i64 0
  %t42 = load %struct.hfa34, ptr @hfa34
  %t43 = load %struct.hfa34, ptr @hfa34
  %t44 = load %struct.hfa34, ptr @hfa34
  %t45 = load %struct.hfa34, ptr @hfa34
  call void (ptr, ...) @myprintf(ptr %t41, ptr @hfa34, ptr @hfa34, ptr @hfa34, ptr @hfa34)
  %t46 = getelementptr [28 x i8], ptr @.str53, i64 0, i64 0
  %t47 = load %struct.hfa33, ptr @hfa33
  %t48 = load %struct.hfa34, ptr @hfa34
  %t49 = load %struct.hfa34, ptr @hfa34
  %t50 = load %struct.hfa34, ptr @hfa34
  call void (ptr, ...) @myprintf(ptr %t46, ptr @hfa33, ptr @hfa34, ptr @hfa34, ptr @hfa34)
  %t51 = getelementptr [28 x i8], ptr @.str54, i64 0, i64 0
  %t52 = load %struct.hfa32, ptr @hfa32
  %t53 = load %struct.hfa34, ptr @hfa34
  %t54 = load %struct.hfa34, ptr @hfa34
  %t55 = load %struct.hfa34, ptr @hfa34
  call void (ptr, ...) @myprintf(ptr %t51, ptr @hfa32, ptr @hfa34, ptr @hfa34, ptr @hfa34)
  %t56 = getelementptr [28 x i8], ptr @.str55, i64 0, i64 0
  %t57 = load %struct.hfa31, ptr @hfa31
  %t58 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t58, ptr @hfa31, i64 16, i1 false)
  %t59 = load [2 x i64], ptr %t58
  %t60 = load %struct.hfa34, ptr @hfa34
  %t61 = load %struct.hfa34, ptr @hfa34
  %t62 = load %struct.hfa34, ptr @hfa34
  call void (ptr, ...) @myprintf(ptr %t56, [2 x i64] %t59, ptr @hfa34, ptr @hfa34, ptr @hfa34)
  %t63 = getelementptr [35 x i8], ptr @.str56, i64 0, i64 0
  %t64 = load %struct.hfa32, ptr @hfa32
  %t65 = load %struct.hfa33, ptr @hfa33
  %t66 = load %struct.hfa33, ptr @hfa33
  %t67 = load %struct.hfa33, ptr @hfa33
  %t68 = load %struct.hfa33, ptr @hfa33
  call void (ptr, ...) @myprintf(ptr %t63, ptr @hfa32, ptr @hfa33, ptr @hfa33, ptr @hfa33, ptr @hfa33)
  %t69 = getelementptr [35 x i8], ptr @.str57, i64 0, i64 0
  %t70 = load %struct.hfa31, ptr @hfa31
  %t71 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t71, ptr @hfa31, i64 16, i1 false)
  %t72 = load [2 x i64], ptr %t71
  %t73 = load %struct.hfa33, ptr @hfa33
  %t74 = load %struct.hfa33, ptr @hfa33
  %t75 = load %struct.hfa33, ptr @hfa33
  %t76 = load %struct.hfa33, ptr @hfa33
  call void (ptr, ...) @myprintf(ptr %t69, [2 x i64] %t72, ptr @hfa33, ptr @hfa33, ptr @hfa33, ptr @hfa33)
  %t77 = getelementptr [28 x i8], ptr @.str58, i64 0, i64 0
  %t78 = load %struct.hfa33, ptr @hfa33
  %t79 = load %struct.hfa33, ptr @hfa33
  %t80 = load %struct.hfa33, ptr @hfa33
  %t81 = load %struct.hfa33, ptr @hfa33
  call void (ptr, ...) @myprintf(ptr %t77, ptr @hfa33, ptr @hfa33, ptr @hfa33, ptr @hfa33)
  %t82 = getelementptr [35 x i8], ptr @.str59, i64 0, i64 0
  %t83 = load %struct.hfa34, ptr @hfa34
  %t84 = load %struct.hfa32, ptr @hfa32
  %t85 = load %struct.hfa32, ptr @hfa32
  %t86 = load %struct.hfa32, ptr @hfa32
  %t87 = load %struct.hfa32, ptr @hfa32
  call void (ptr, ...) @myprintf(ptr %t82, ptr @hfa34, ptr @hfa32, ptr @hfa32, ptr @hfa32, ptr @hfa32)
  %t88 = getelementptr [35 x i8], ptr @.str60, i64 0, i64 0
  %t89 = load %struct.hfa33, ptr @hfa33
  %t90 = load %struct.hfa32, ptr @hfa32
  %t91 = load %struct.hfa32, ptr @hfa32
  %t92 = load %struct.hfa32, ptr @hfa32
  %t93 = load %struct.hfa32, ptr @hfa32
  call void (ptr, ...) @myprintf(ptr %t88, ptr @hfa33, ptr @hfa32, ptr @hfa32, ptr @hfa32, ptr @hfa32)
  %t94 = getelementptr [42 x i8], ptr @.str61, i64 0, i64 0
  %t95 = load %struct.hfa34, ptr @hfa34
  %t96 = load %struct.hfa32, ptr @hfa32
  %t97 = load %struct.hfa31, ptr @hfa31
  %t98 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t98, ptr @hfa31, i64 16, i1 false)
  %t99 = load [2 x i64], ptr %t98
  %t100 = load %struct.hfa31, ptr @hfa31
  %t101 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t101, ptr @hfa31, i64 16, i1 false)
  %t102 = load [2 x i64], ptr %t101
  %t103 = load %struct.hfa31, ptr @hfa31
  %t104 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t104, ptr @hfa31, i64 16, i1 false)
  %t105 = load [2 x i64], ptr %t104
  %t106 = load %struct.hfa31, ptr @hfa31
  %t107 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t107, ptr @hfa31, i64 16, i1 false)
  %t108 = load [2 x i64], ptr %t107
  call void (ptr, ...) @myprintf(ptr %t94, ptr @hfa34, ptr @hfa32, [2 x i64] %t99, [2 x i64] %t102, [2 x i64] %t105, [2 x i64] %t108)
  %t109 = getelementptr [12 x i8], ptr @.str62, i64 0, i64 0
  call void (ptr, ...) @myprintf(ptr %t109)
  %t110 = getelementptr [28 x i8], ptr @.str63, i64 0, i64 0
  %t111 = load %struct.hfa24, ptr @hfa24
  %t112 = load %struct.hfa24, ptr @hfa24
  %t113 = load %struct.hfa24, ptr @hfa24
  %t114 = load %struct.hfa24, ptr @hfa24
  call void (ptr, ...) @myprintf(ptr %t110, ptr @hfa24, ptr @hfa24, ptr @hfa24, ptr @hfa24)
  %t115 = getelementptr [28 x i8], ptr @.str64, i64 0, i64 0
  %t116 = load %struct.hfa23, ptr @hfa23
  %t117 = load %struct.hfa24, ptr @hfa24
  %t118 = load %struct.hfa24, ptr @hfa24
  %t119 = load %struct.hfa24, ptr @hfa24
  call void (ptr, ...) @myprintf(ptr %t115, ptr @hfa23, ptr @hfa24, ptr @hfa24, ptr @hfa24)
  %t120 = getelementptr [28 x i8], ptr @.str65, i64 0, i64 0
  %t121 = load %struct.hfa22, ptr @hfa22
  %t122 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t122, ptr @hfa22, i64 16, i1 false)
  %t123 = load [2 x i64], ptr %t122
  %t124 = load %struct.hfa24, ptr @hfa24
  %t125 = load %struct.hfa24, ptr @hfa24
  %t126 = load %struct.hfa24, ptr @hfa24
  call void (ptr, ...) @myprintf(ptr %t120, [2 x i64] %t123, ptr @hfa24, ptr @hfa24, ptr @hfa24)
  %t127 = getelementptr [28 x i8], ptr @.str66, i64 0, i64 0
  %t128 = load %struct.hfa21, ptr @hfa21
  %t129 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t129, ptr @hfa21, i64 8, i1 false)
  %t130 = load i64, ptr %t129
  %t131 = load %struct.hfa24, ptr @hfa24
  %t132 = load %struct.hfa24, ptr @hfa24
  %t133 = load %struct.hfa24, ptr @hfa24
  call void (ptr, ...) @myprintf(ptr %t127, i64 %t130, ptr @hfa24, ptr @hfa24, ptr @hfa24)
  %t134 = getelementptr [35 x i8], ptr @.str67, i64 0, i64 0
  %t135 = load %struct.hfa22, ptr @hfa22
  %t136 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t136, ptr @hfa22, i64 16, i1 false)
  %t137 = load [2 x i64], ptr %t136
  %t138 = load %struct.hfa23, ptr @hfa23
  %t139 = load %struct.hfa23, ptr @hfa23
  %t140 = load %struct.hfa23, ptr @hfa23
  %t141 = load %struct.hfa23, ptr @hfa23
  call void (ptr, ...) @myprintf(ptr %t134, [2 x i64] %t137, ptr @hfa23, ptr @hfa23, ptr @hfa23, ptr @hfa23)
  %t142 = getelementptr [35 x i8], ptr @.str68, i64 0, i64 0
  %t143 = load %struct.hfa21, ptr @hfa21
  %t144 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t144, ptr @hfa21, i64 8, i1 false)
  %t145 = load i64, ptr %t144
  %t146 = load %struct.hfa23, ptr @hfa23
  %t147 = load %struct.hfa23, ptr @hfa23
  %t148 = load %struct.hfa23, ptr @hfa23
  %t149 = load %struct.hfa23, ptr @hfa23
  call void (ptr, ...) @myprintf(ptr %t142, i64 %t145, ptr @hfa23, ptr @hfa23, ptr @hfa23, ptr @hfa23)
  %t150 = getelementptr [28 x i8], ptr @.str69, i64 0, i64 0
  %t151 = load %struct.hfa23, ptr @hfa23
  %t152 = load %struct.hfa23, ptr @hfa23
  %t153 = load %struct.hfa23, ptr @hfa23
  %t154 = load %struct.hfa23, ptr @hfa23
  call void (ptr, ...) @myprintf(ptr %t150, ptr @hfa23, ptr @hfa23, ptr @hfa23, ptr @hfa23)
  %t155 = getelementptr [35 x i8], ptr @.str70, i64 0, i64 0
  %t156 = load %struct.hfa24, ptr @hfa24
  %t157 = load %struct.hfa22, ptr @hfa22
  %t158 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t158, ptr @hfa22, i64 16, i1 false)
  %t159 = load [2 x i64], ptr %t158
  %t160 = load %struct.hfa22, ptr @hfa22
  %t161 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t161, ptr @hfa22, i64 16, i1 false)
  %t162 = load [2 x i64], ptr %t161
  %t163 = load %struct.hfa22, ptr @hfa22
  %t164 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t164, ptr @hfa22, i64 16, i1 false)
  %t165 = load [2 x i64], ptr %t164
  %t166 = load %struct.hfa22, ptr @hfa22
  %t167 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t167, ptr @hfa22, i64 16, i1 false)
  %t168 = load [2 x i64], ptr %t167
  call void (ptr, ...) @myprintf(ptr %t155, ptr @hfa24, [2 x i64] %t159, [2 x i64] %t162, [2 x i64] %t165, [2 x i64] %t168)
  %t169 = getelementptr [35 x i8], ptr @.str71, i64 0, i64 0
  %t170 = load %struct.hfa23, ptr @hfa23
  %t171 = load %struct.hfa22, ptr @hfa22
  %t172 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t172, ptr @hfa22, i64 16, i1 false)
  %t173 = load [2 x i64], ptr %t172
  %t174 = load %struct.hfa22, ptr @hfa22
  %t175 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t175, ptr @hfa22, i64 16, i1 false)
  %t176 = load [2 x i64], ptr %t175
  %t177 = load %struct.hfa22, ptr @hfa22
  %t178 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t178, ptr @hfa22, i64 16, i1 false)
  %t179 = load [2 x i64], ptr %t178
  %t180 = load %struct.hfa22, ptr @hfa22
  %t181 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t181, ptr @hfa22, i64 16, i1 false)
  %t182 = load [2 x i64], ptr %t181
  call void (ptr, ...) @myprintf(ptr %t169, ptr @hfa23, [2 x i64] %t173, [2 x i64] %t176, [2 x i64] %t179, [2 x i64] %t182)
  %t183 = getelementptr [42 x i8], ptr @.str72, i64 0, i64 0
  %t184 = load %struct.hfa24, ptr @hfa24
  %t185 = load %struct.hfa22, ptr @hfa22
  %t186 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t186, ptr @hfa22, i64 16, i1 false)
  %t187 = load [2 x i64], ptr %t186
  %t188 = load %struct.hfa21, ptr @hfa21
  %t189 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t189, ptr @hfa21, i64 8, i1 false)
  %t190 = load i64, ptr %t189
  %t191 = load %struct.hfa21, ptr @hfa21
  %t192 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t192, ptr @hfa21, i64 8, i1 false)
  %t193 = load i64, ptr %t192
  %t194 = load %struct.hfa21, ptr @hfa21
  %t195 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t195, ptr @hfa21, i64 8, i1 false)
  %t196 = load i64, ptr %t195
  %t197 = load %struct.hfa21, ptr @hfa21
  %t198 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t198, ptr @hfa21, i64 8, i1 false)
  %t199 = load i64, ptr %t198
  call void (ptr, ...) @myprintf(ptr %t183, ptr @hfa24, [2 x i64] %t187, i64 %t190, i64 %t193, i64 %t196, i64 %t199)
  %t200 = getelementptr [11 x i8], ptr @.str73, i64 0, i64 0
  call void (ptr, ...) @myprintf(ptr %t200)
  %t201 = getelementptr [28 x i8], ptr @.str74, i64 0, i64 0
  %t202 = load %struct.hfa14, ptr @hfa14
  %t203 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t203, ptr @hfa14, i64 16, i1 false)
  %t204 = load [2 x i64], ptr %t203
  %t205 = load %struct.hfa14, ptr @hfa14
  %t206 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t206, ptr @hfa14, i64 16, i1 false)
  %t207 = load [2 x i64], ptr %t206
  %t208 = load %struct.hfa14, ptr @hfa14
  %t209 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t209, ptr @hfa14, i64 16, i1 false)
  %t210 = load [2 x i64], ptr %t209
  %t211 = load %struct.hfa14, ptr @hfa14
  %t212 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t212, ptr @hfa14, i64 16, i1 false)
  %t213 = load [2 x i64], ptr %t212
  call void (ptr, ...) @myprintf(ptr %t201, [2 x i64] %t204, [2 x i64] %t207, [2 x i64] %t210, [2 x i64] %t213)
  %t214 = getelementptr [28 x i8], ptr @.str75, i64 0, i64 0
  %t215 = load %struct.hfa13, ptr @hfa13
  %t216 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t216, ptr @hfa13, i64 12, i1 false)
  %t217 = load [2 x i64], ptr %t216
  %t218 = load %struct.hfa14, ptr @hfa14
  %t219 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t219, ptr @hfa14, i64 16, i1 false)
  %t220 = load [2 x i64], ptr %t219
  %t221 = load %struct.hfa14, ptr @hfa14
  %t222 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t222, ptr @hfa14, i64 16, i1 false)
  %t223 = load [2 x i64], ptr %t222
  %t224 = load %struct.hfa14, ptr @hfa14
  %t225 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t225, ptr @hfa14, i64 16, i1 false)
  %t226 = load [2 x i64], ptr %t225
  call void (ptr, ...) @myprintf(ptr %t214, [2 x i64] %t217, [2 x i64] %t220, [2 x i64] %t223, [2 x i64] %t226)
  %t227 = getelementptr [28 x i8], ptr @.str76, i64 0, i64 0
  %t228 = load %struct.hfa12, ptr @hfa12
  %t229 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t229, ptr @hfa12, i64 8, i1 false)
  %t230 = load i64, ptr %t229
  %t231 = load %struct.hfa14, ptr @hfa14
  %t232 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t232, ptr @hfa14, i64 16, i1 false)
  %t233 = load [2 x i64], ptr %t232
  %t234 = load %struct.hfa14, ptr @hfa14
  %t235 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t235, ptr @hfa14, i64 16, i1 false)
  %t236 = load [2 x i64], ptr %t235
  %t237 = load %struct.hfa14, ptr @hfa14
  %t238 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t238, ptr @hfa14, i64 16, i1 false)
  %t239 = load [2 x i64], ptr %t238
  call void (ptr, ...) @myprintf(ptr %t227, i64 %t230, [2 x i64] %t233, [2 x i64] %t236, [2 x i64] %t239)
  %t240 = getelementptr [28 x i8], ptr @.str77, i64 0, i64 0
  %t241 = load %struct.hfa11, ptr @hfa11
  %t242 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t242, ptr @hfa11, i64 4, i1 false)
  %t243 = load i64, ptr %t242
  %t244 = load %struct.hfa14, ptr @hfa14
  %t245 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t245, ptr @hfa14, i64 16, i1 false)
  %t246 = load [2 x i64], ptr %t245
  %t247 = load %struct.hfa14, ptr @hfa14
  %t248 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t248, ptr @hfa14, i64 16, i1 false)
  %t249 = load [2 x i64], ptr %t248
  %t250 = load %struct.hfa14, ptr @hfa14
  %t251 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t251, ptr @hfa14, i64 16, i1 false)
  %t252 = load [2 x i64], ptr %t251
  call void (ptr, ...) @myprintf(ptr %t240, i64 %t243, [2 x i64] %t246, [2 x i64] %t249, [2 x i64] %t252)
  %t253 = getelementptr [35 x i8], ptr @.str78, i64 0, i64 0
  %t254 = load %struct.hfa12, ptr @hfa12
  %t255 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t255, ptr @hfa12, i64 8, i1 false)
  %t256 = load i64, ptr %t255
  %t257 = load %struct.hfa13, ptr @hfa13
  %t258 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t258, ptr @hfa13, i64 12, i1 false)
  %t259 = load [2 x i64], ptr %t258
  %t260 = load %struct.hfa13, ptr @hfa13
  %t261 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t261, ptr @hfa13, i64 12, i1 false)
  %t262 = load [2 x i64], ptr %t261
  %t263 = load %struct.hfa13, ptr @hfa13
  %t264 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t264, ptr @hfa13, i64 12, i1 false)
  %t265 = load [2 x i64], ptr %t264
  %t266 = load %struct.hfa13, ptr @hfa13
  %t267 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t267, ptr @hfa13, i64 12, i1 false)
  %t268 = load [2 x i64], ptr %t267
  call void (ptr, ...) @myprintf(ptr %t253, i64 %t256, [2 x i64] %t259, [2 x i64] %t262, [2 x i64] %t265, [2 x i64] %t268)
  %t269 = getelementptr [35 x i8], ptr @.str79, i64 0, i64 0
  %t270 = load %struct.hfa11, ptr @hfa11
  %t271 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t271, ptr @hfa11, i64 4, i1 false)
  %t272 = load i64, ptr %t271
  %t273 = load %struct.hfa13, ptr @hfa13
  %t274 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t274, ptr @hfa13, i64 12, i1 false)
  %t275 = load [2 x i64], ptr %t274
  %t276 = load %struct.hfa13, ptr @hfa13
  %t277 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t277, ptr @hfa13, i64 12, i1 false)
  %t278 = load [2 x i64], ptr %t277
  %t279 = load %struct.hfa13, ptr @hfa13
  %t280 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t280, ptr @hfa13, i64 12, i1 false)
  %t281 = load [2 x i64], ptr %t280
  %t282 = load %struct.hfa13, ptr @hfa13
  %t283 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t283, ptr @hfa13, i64 12, i1 false)
  %t284 = load [2 x i64], ptr %t283
  call void (ptr, ...) @myprintf(ptr %t269, i64 %t272, [2 x i64] %t275, [2 x i64] %t278, [2 x i64] %t281, [2 x i64] %t284)
  %t285 = getelementptr [28 x i8], ptr @.str80, i64 0, i64 0
  %t286 = load %struct.hfa13, ptr @hfa13
  %t287 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t287, ptr @hfa13, i64 12, i1 false)
  %t288 = load [2 x i64], ptr %t287
  %t289 = load %struct.hfa13, ptr @hfa13
  %t290 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t290, ptr @hfa13, i64 12, i1 false)
  %t291 = load [2 x i64], ptr %t290
  %t292 = load %struct.hfa13, ptr @hfa13
  %t293 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t293, ptr @hfa13, i64 12, i1 false)
  %t294 = load [2 x i64], ptr %t293
  %t295 = load %struct.hfa13, ptr @hfa13
  %t296 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t296, ptr @hfa13, i64 12, i1 false)
  %t297 = load [2 x i64], ptr %t296
  call void (ptr, ...) @myprintf(ptr %t285, [2 x i64] %t288, [2 x i64] %t291, [2 x i64] %t294, [2 x i64] %t297)
  %t298 = getelementptr [35 x i8], ptr @.str81, i64 0, i64 0
  %t299 = load %struct.hfa14, ptr @hfa14
  %t300 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t300, ptr @hfa14, i64 16, i1 false)
  %t301 = load [2 x i64], ptr %t300
  %t302 = load %struct.hfa12, ptr @hfa12
  %t303 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t303, ptr @hfa12, i64 8, i1 false)
  %t304 = load i64, ptr %t303
  %t305 = load %struct.hfa12, ptr @hfa12
  %t306 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t306, ptr @hfa12, i64 8, i1 false)
  %t307 = load i64, ptr %t306
  %t308 = load %struct.hfa12, ptr @hfa12
  %t309 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t309, ptr @hfa12, i64 8, i1 false)
  %t310 = load i64, ptr %t309
  %t311 = load %struct.hfa12, ptr @hfa12
  %t312 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t312, ptr @hfa12, i64 8, i1 false)
  %t313 = load i64, ptr %t312
  call void (ptr, ...) @myprintf(ptr %t298, [2 x i64] %t301, i64 %t304, i64 %t307, i64 %t310, i64 %t313)
  %t314 = getelementptr [35 x i8], ptr @.str82, i64 0, i64 0
  %t315 = load %struct.hfa13, ptr @hfa13
  %t316 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t316, ptr @hfa13, i64 12, i1 false)
  %t317 = load [2 x i64], ptr %t316
  %t318 = load %struct.hfa12, ptr @hfa12
  %t319 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t319, ptr @hfa12, i64 8, i1 false)
  %t320 = load i64, ptr %t319
  %t321 = load %struct.hfa12, ptr @hfa12
  %t322 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t322, ptr @hfa12, i64 8, i1 false)
  %t323 = load i64, ptr %t322
  %t324 = load %struct.hfa12, ptr @hfa12
  %t325 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t325, ptr @hfa12, i64 8, i1 false)
  %t326 = load i64, ptr %t325
  %t327 = load %struct.hfa12, ptr @hfa12
  %t328 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t328, ptr @hfa12, i64 8, i1 false)
  %t329 = load i64, ptr %t328
  call void (ptr, ...) @myprintf(ptr %t314, [2 x i64] %t317, i64 %t320, i64 %t323, i64 %t326, i64 %t329)
  %t330 = getelementptr [42 x i8], ptr @.str83, i64 0, i64 0
  %t331 = load %struct.hfa14, ptr @hfa14
  %t332 = alloca [2 x i64]
  call void @llvm.memcpy.p0.p0.i64(ptr %t332, ptr @hfa14, i64 16, i1 false)
  %t333 = load [2 x i64], ptr %t332
  %t334 = load %struct.hfa12, ptr @hfa12
  %t335 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t335, ptr @hfa12, i64 8, i1 false)
  %t336 = load i64, ptr %t335
  %t337 = load %struct.hfa11, ptr @hfa11
  %t338 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t338, ptr @hfa11, i64 4, i1 false)
  %t339 = load i64, ptr %t338
  %t340 = load %struct.hfa11, ptr @hfa11
  %t341 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t341, ptr @hfa11, i64 4, i1 false)
  %t342 = load i64, ptr %t341
  %t343 = load %struct.hfa11, ptr @hfa11
  %t344 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t344, ptr @hfa11, i64 4, i1 false)
  %t345 = load i64, ptr %t344
  %t346 = load %struct.hfa11, ptr @hfa11
  %t347 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t347, ptr @hfa11, i64 4, i1 false)
  %t348 = load i64, ptr %t347
  call void (ptr, ...) @myprintf(ptr %t330, [2 x i64] %t333, i64 %t336, i64 %t339, i64 %t342, i64 %t345, i64 %t348)
  ret void
}

define void @pll(i64 %p.x)
{
entry:
  %t0 = getelementptr [6 x i8], ptr @.str84, i64 0, i64 0
  %t1 = call i32 (ptr, ...) @printf(ptr %t0, i64 %p.x)
  ret void
}

define void @movi()
{
entry:
  %t0 = getelementptr [7 x i8], ptr @.str85, i64 0, i64 0
  %t1 = call i32 (ptr, ...) @printf(ptr %t0)
  %t2 = sext i32 0 to i64
  call void (i64) @pll(i64 %t2)
  %t3 = sext i32 43981 to i64
  call void (i64) @pll(i64 %t3)
  %t4 = zext i32 2882338816 to i64
  call void (i64) @pll(i64 %t4)
  call void (i64) @pll(i64 188896956645376)
  call void (i64) @pll(i64 -6067193122998190080)
  %t5 = zext i32 4294945741 to i64
  call void (i64) @pll(i64 %t5)
  %t6 = zext i32 2882404351 to i64
  call void (i64) @pll(i64 %t6)
  call void (i64) @pll(i64 -21555)
  call void (i64) @pll(i64 -1412562945)
  call void (i64) @pll(i64 -92573725097985)
  call void (i64) @pll(i64 -6066911648021479425)
  %t7 = zext i32 2863311530 to i64
  call void (i64) @pll(i64 %t7)
  call void (i64) @pll(i64 6148914691236517205)
  %t8 = sext i32 2004318071 to i64
  call void (i64) @pll(i64 %t8)
  call void (i64) @pll(i64 3689348814741910323)
  %t9 = zext i32 4177066232 to i64
  call void (i64) @pll(i64 %t9)
  call void (i64) @pll(i64 2170205185142300190)
  %t10 = sext i32 1065369472 to i64
  call void (i64) @pll(i64 %t10)
  call void (i64) @pll(i64 143835907860922879)
  %t11 = sext i32 8388544 to i64
  call void (i64) @pll(i64 %t11)
  call void (i64) @pll(i64 288221580125796352)
  call void (i64) @pll(i64 2251799813684736)
  %t12 = zext i32 2882343476 to i64
  call void (i64) @pll(i64 %t12)
  call void (i64) @pll(i64 188896956650036)
  call void (i64) @pll(i64 -6067193122998185420)
  call void (i64) @pll(i64 188897262043136)
  call void (i64) @pll(i64 -6067193122692792320)
  call void (i64) @pll(i64 -6067173108450590720)
  call void (i64) @pll(i64 -1412623820)
  call void (i64) @pll(i64 -92573725158860)
  call void (i64) @pll(i64 -6066911648021540300)
  call void (i64) @pll(i64 -92577714601985)
  call void (i64) @pll(i64 -6066911652010983425)
  call void (i64) @pll(i64 -6067173104155623425)
  call void (i64) @pll(i64 -18686810953847)
  call void (i64) @pll(i64 -6066930334832394241)
  call void (i64) @pll(i64 -6066930334832433271)
  ret void
}

define internal i32 @addip0(i32 %p.x)
{
entry:
  %t0 = add i32 %p.x, 0
  ret i32 %t0
}

define internal i64 @sublp0(i64 %p.x)
{
entry:
  %t0 = sext i32 0 to i64
  %t1 = sub i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @addip123(i32 %p.x)
{
entry:
  %t0 = add i32 %p.x, 123
  ret i32 %t0
}

define internal i64 @addlm123(i64 %p.x)
{
entry:
  %t0 = sub i32 0, 123
  %t1 = sext i32 %t0 to i64
  %t2 = add i64 %p.x, %t1
  ret i64 %t2
}

define internal i64 @sublp4095(i64 %p.x)
{
entry:
  %t0 = sext i32 4095 to i64
  %t1 = sub i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @subim503808(i32 %p.x)
{
entry:
  %t0 = sub i32 0, 503808
  %t1 = sub i32 %p.x, %t0
  ret i32 %t1
}

define internal i64 @addp12345(i64 %p.x)
{
entry:
  %t0 = sext i32 12345 to i64
  %t1 = add i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @subp12345(i32 %p.x)
{
entry:
  %t0 = sub i32 %p.x, 12345
  ret i32 %t0
}

define internal i32 @mvni(i32 %p.x)
{
entry:
  %t0 = sub i32 4294967295, %p.x
  ret i32 %t0
}

define internal i64 @negl(i64 %p.x)
{
entry:
  %t0 = sext i32 0 to i64
  %t1 = sub i64 %t0, %p.x
  ret i64 %t1
}

define internal i32 @rsbi123(i32 %p.x)
{
entry:
  %t0 = sub i32 123, %p.x
  ret i32 %t0
}

define internal i64 @rsbl123(i64 %p.x)
{
entry:
  %t0 = sext i32 123 to i64
  %t1 = sub i64 %t0, %p.x
  ret i64 %t1
}

define internal i32 @andi0(i32 %p.x)
{
entry:
  %t0 = and i32 %p.x, 0
  ret i32 %t0
}

define internal i64 @andlm1(i64 %p.x)
{
entry:
  %t0 = sub i32 0, 1
  %t1 = sext i32 %t0 to i64
  %t2 = and i64 %p.x, %t1
  ret i64 %t2
}

define internal i64 @orrl0(i64 %p.x)
{
entry:
  %t0 = sext i32 0 to i64
  %t1 = or i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @orrim1(i32 %p.x)
{
entry:
  %t0 = sub i32 0, 1
  %t1 = or i32 %p.x, %t0
  ret i32 %t1
}

define internal i32 @eori0(i32 %p.x)
{
entry:
  %t0 = xor i32 %p.x, 0
  ret i32 %t0
}

define internal i64 @eorlm1(i64 %p.x)
{
entry:
  %t0 = sub i32 0, 1
  %t1 = sext i32 %t0 to i64
  %t2 = xor i64 %p.x, %t1
  ret i64 %t2
}

define internal i32 @and0xf0(i32 %p.x)
{
entry:
  %t0 = and i32 %p.x, 240
  ret i32 %t0
}

define internal i64 @orr0xf0(i64 %p.x)
{
entry:
  %t0 = sext i32 240 to i64
  %t1 = or i64 %p.x, %t0
  ret i64 %t1
}

define internal i64 @eor0xf0(i64 %p.x)
{
entry:
  %t0 = sext i32 240 to i64
  %t1 = xor i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @lsli0(i32 %p.x)
{
entry:
  %t0 = shl i32 %p.x, 0
  ret i32 %t0
}

define internal i32 @lsri0(i32 %p.x)
{
entry:
  %t0 = lshr i32 %p.x, 0
  ret i32 %t0
}

define internal i64 @asrl0(i64 %p.x)
{
entry:
  %t0 = sext i32 0 to i64
  %t1 = ashr i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @lsli1(i32 %p.x)
{
entry:
  %t0 = shl i32 %p.x, 1
  ret i32 %t0
}

define internal i32 @lsli31(i32 %p.x)
{
entry:
  %t0 = shl i32 %p.x, 31
  ret i32 %t0
}

define internal i64 @lsll1(i64 %p.x)
{
entry:
  %t0 = sext i32 1 to i64
  %t1 = shl i64 %p.x, %t0
  ret i64 %t1
}

define internal i64 @lsll63(i64 %p.x)
{
entry:
  %t0 = sext i32 63 to i64
  %t1 = shl i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @lsri1(i32 %p.x)
{
entry:
  %t0 = lshr i32 %p.x, 1
  ret i32 %t0
}

define internal i32 @lsri31(i32 %p.x)
{
entry:
  %t0 = lshr i32 %p.x, 31
  ret i32 %t0
}

define internal i64 @lsrl1(i64 %p.x)
{
entry:
  %t0 = sext i32 1 to i64
  %t1 = lshr i64 %p.x, %t0
  ret i64 %t1
}

define internal i64 @lsrl63(i64 %p.x)
{
entry:
  %t0 = sext i32 63 to i64
  %t1 = lshr i64 %p.x, %t0
  ret i64 %t1
}

define internal i32 @asri1(i32 %p.x)
{
entry:
  %t0 = ashr i32 %p.x, 1
  ret i32 %t0
}

define internal i32 @asri31(i32 %p.x)
{
entry:
  %t0 = ashr i32 %p.x, 31
  ret i32 %t0
}

define internal i64 @asrl1(i64 %p.x)
{
entry:
  %t0 = sext i32 1 to i64
  %t1 = ashr i64 %p.x, %t0
  ret i64 %t1
}

define internal i64 @asrl63(i64 %p.x)
{
entry:
  %t0 = sext i32 63 to i64
  %t1 = ashr i64 %p.x, %t0
  ret i64 %t1
}

define void @opi()
{
entry:
  %lv.x = alloca i32, align 4
  store i32 1000, ptr %lv.x
  %t0 = load i32, ptr %lv.x
  %t1 = call i32 (i32) @addip0(i32 %t0)
  %t2 = zext i32 %t1 to i64
  call void (i64) @pll(i64 %t2)
  %t3 = load i32, ptr %lv.x
  %t4 = sext i32 %t3 to i64
  %t5 = call i64 (i64) @sublp0(i64 %t4)
  call void (i64) @pll(i64 %t5)
  %t6 = load i32, ptr %lv.x
  %t7 = call i32 (i32) @addip123(i32 %t6)
  %t8 = zext i32 %t7 to i64
  call void (i64) @pll(i64 %t8)
  %t9 = load i32, ptr %lv.x
  %t10 = sext i32 %t9 to i64
  %t11 = call i64 (i64) @addlm123(i64 %t10)
  call void (i64) @pll(i64 %t11)
  %t12 = load i32, ptr %lv.x
  %t13 = sext i32 %t12 to i64
  %t14 = call i64 (i64) @sublp4095(i64 %t13)
  call void (i64) @pll(i64 %t14)
  %t15 = load i32, ptr %lv.x
  %t16 = call i32 (i32) @subim503808(i32 %t15)
  %t17 = zext i32 %t16 to i64
  call void (i64) @pll(i64 %t17)
  %t18 = load i32, ptr %lv.x
  %t19 = sext i32 %t18 to i64
  %t20 = call i64 (i64) @addp12345(i64 %t19)
  call void (i64) @pll(i64 %t20)
  %t21 = load i32, ptr %lv.x
  %t22 = call i32 (i32) @subp12345(i32 %t21)
  %t23 = zext i32 %t22 to i64
  call void (i64) @pll(i64 %t23)
  %t24 = load i32, ptr %lv.x
  %t25 = call i32 (i32) @mvni(i32 %t24)
  %t26 = zext i32 %t25 to i64
  call void (i64) @pll(i64 %t26)
  %t27 = load i32, ptr %lv.x
  %t28 = sext i32 %t27 to i64
  %t29 = call i64 (i64) @negl(i64 %t28)
  call void (i64) @pll(i64 %t29)
  %t30 = load i32, ptr %lv.x
  %t31 = call i32 (i32) @rsbi123(i32 %t30)
  %t32 = zext i32 %t31 to i64
  call void (i64) @pll(i64 %t32)
  %t33 = load i32, ptr %lv.x
  %t34 = sext i32 %t33 to i64
  %t35 = call i64 (i64) @rsbl123(i64 %t34)
  call void (i64) @pll(i64 %t35)
  %t36 = load i32, ptr %lv.x
  %t37 = call i32 (i32) @andi0(i32 %t36)
  %t38 = zext i32 %t37 to i64
  call void (i64) @pll(i64 %t38)
  %t39 = load i32, ptr %lv.x
  %t40 = sext i32 %t39 to i64
  %t41 = call i64 (i64) @andlm1(i64 %t40)
  call void (i64) @pll(i64 %t41)
  %t42 = load i32, ptr %lv.x
  %t43 = sext i32 %t42 to i64
  %t44 = call i64 (i64) @orrl0(i64 %t43)
  call void (i64) @pll(i64 %t44)
  %t45 = load i32, ptr %lv.x
  %t46 = call i32 (i32) @orrim1(i32 %t45)
  %t47 = zext i32 %t46 to i64
  call void (i64) @pll(i64 %t47)
  %t48 = load i32, ptr %lv.x
  %t49 = call i32 (i32) @eori0(i32 %t48)
  %t50 = zext i32 %t49 to i64
  call void (i64) @pll(i64 %t50)
  %t51 = load i32, ptr %lv.x
  %t52 = sext i32 %t51 to i64
  %t53 = call i64 (i64) @eorlm1(i64 %t52)
  call void (i64) @pll(i64 %t53)
  %t54 = load i32, ptr %lv.x
  %t55 = call i32 (i32) @and0xf0(i32 %t54)
  %t56 = zext i32 %t55 to i64
  call void (i64) @pll(i64 %t56)
  %t57 = load i32, ptr %lv.x
  %t58 = sext i32 %t57 to i64
  %t59 = call i64 (i64) @orr0xf0(i64 %t58)
  call void (i64) @pll(i64 %t59)
  %t60 = load i32, ptr %lv.x
  %t61 = sext i32 %t60 to i64
  %t62 = call i64 (i64) @eor0xf0(i64 %t61)
  call void (i64) @pll(i64 %t62)
  %t63 = load i32, ptr %lv.x
  %t64 = call i32 (i32) @lsli0(i32 %t63)
  %t65 = zext i32 %t64 to i64
  call void (i64) @pll(i64 %t65)
  %t66 = load i32, ptr %lv.x
  %t67 = call i32 (i32) @lsri0(i32 %t66)
  %t68 = zext i32 %t67 to i64
  call void (i64) @pll(i64 %t68)
  %t69 = load i32, ptr %lv.x
  %t70 = sext i32 %t69 to i64
  %t71 = call i64 (i64) @asrl0(i64 %t70)
  call void (i64) @pll(i64 %t71)
  %t72 = load i32, ptr %lv.x
  %t73 = call i32 (i32) @lsli1(i32 %t72)
  %t74 = zext i32 %t73 to i64
  call void (i64) @pll(i64 %t74)
  %t75 = load i32, ptr %lv.x
  %t76 = call i32 (i32) @lsli31(i32 %t75)
  %t77 = zext i32 %t76 to i64
  call void (i64) @pll(i64 %t77)
  %t78 = load i32, ptr %lv.x
  %t79 = sext i32 %t78 to i64
  %t80 = call i64 (i64) @lsll1(i64 %t79)
  call void (i64) @pll(i64 %t80)
  %t81 = load i32, ptr %lv.x
  %t82 = sext i32 %t81 to i64
  %t83 = call i64 (i64) @lsll63(i64 %t82)
  call void (i64) @pll(i64 %t83)
  %t84 = load i32, ptr %lv.x
  %t85 = call i32 (i32) @lsri1(i32 %t84)
  %t86 = zext i32 %t85 to i64
  call void (i64) @pll(i64 %t86)
  %t87 = load i32, ptr %lv.x
  %t88 = call i32 (i32) @lsri31(i32 %t87)
  %t89 = zext i32 %t88 to i64
  call void (i64) @pll(i64 %t89)
  %t90 = load i32, ptr %lv.x
  %t91 = sext i32 %t90 to i64
  %t92 = call i64 (i64) @lsrl1(i64 %t91)
  call void (i64) @pll(i64 %t92)
  %t93 = load i32, ptr %lv.x
  %t94 = sext i32 %t93 to i64
  %t95 = call i64 (i64) @lsrl63(i64 %t94)
  call void (i64) @pll(i64 %t95)
  %t96 = load i32, ptr %lv.x
  %t97 = call i32 (i32) @asri1(i32 %t96)
  %t98 = sext i32 %t97 to i64
  call void (i64) @pll(i64 %t98)
  %t99 = load i32, ptr %lv.x
  %t100 = call i32 (i32) @asri31(i32 %t99)
  %t101 = sext i32 %t100 to i64
  call void (i64) @pll(i64 %t101)
  %t102 = load i32, ptr %lv.x
  %t103 = sext i32 %t102 to i64
  %t104 = call i64 (i64) @asrl1(i64 %t103)
  call void (i64) @pll(i64 %t104)
  %t105 = load i32, ptr %lv.x
  %t106 = sext i32 %t105 to i64
  %t107 = call i64 (i64) @asrl63(i64 %t106)
  call void (i64) @pll(i64 %t107)
  ret void
}

define void @pcs()
{
entry:
  call void () @arg()
  call void () @ret()
  call void () @stdarg()
  call void () @movi()
  call void () @opi()
  ret void
}

define i32 @main()
{
entry:
  call void () @pcs()
  ret i32 0
}

