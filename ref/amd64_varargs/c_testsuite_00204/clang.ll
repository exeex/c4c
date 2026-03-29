; ModuleID = 'tests/c/external/c-testsuite/src/00204.c'
source_filename = "tests/c/external/c-testsuite/src/00204.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

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
%struct.hfa31 = type { x86_fp80 }
%struct.hfa32 = type { x86_fp80, x86_fp80 }
%struct.hfa33 = type { x86_fp80, x86_fp80, x86_fp80 }
%struct.hfa34 = type { x86_fp80, x86_fp80, x86_fp80, x86_fp80 }
%struct.__va_list_tag = type { i32, i32, ptr, ptr }

@s1 = dso_local global %struct.s1 { [1 x i8] c"0" }, align 1
@s2 = dso_local global %struct.s2 { [2 x i8] c"12" }, align 1
@s3 = dso_local global %struct.s3 { [3 x i8] c"345" }, align 1
@s4 = dso_local global %struct.s4 { [4 x i8] c"6789" }, align 1
@s5 = dso_local global %struct.s5 { [5 x i8] c"abcde" }, align 1
@s6 = dso_local global %struct.s6 { [6 x i8] c"fghijk" }, align 1
@s7 = dso_local global %struct.s7 { [7 x i8] c"lmnopqr" }, align 1
@s8 = dso_local global %struct.s8 { [8 x i8] c"stuvwxyz" }, align 1
@s9 = dso_local global %struct.s9 { [9 x i8] c"ABCDEFGHI" }, align 8
@s10 = dso_local global %struct.s10 { [10 x i8] c"JKLMNOPQRS" }, align 1
@s11 = dso_local global %struct.s11 { [11 x i8] c"TUVWXYZ0123" }, align 8
@s12 = dso_local global %struct.s12 { [12 x i8] c"456789abcdef" }, align 8
@s13 = dso_local global %struct.s13 { [13 x i8] c"ghijklmnopqrs" }, align 8
@s14 = dso_local global %struct.s14 { [14 x i8] c"tuvwxyzABCDEFG" }, align 8
@s15 = dso_local global %struct.s15 { [15 x i8] c"HIJKLMNOPQRSTUV" }, align 1
@s16 = dso_local global %struct.s16 { [16 x i8] c"WXYZ0123456789ab" }, align 1
@s17 = dso_local global %struct.s17 { [17 x i8] c"cdefghijklmnopqrs" }, align 8
@hfa11 = dso_local global %struct.hfa11 { float 0x4026333340000000 }, align 4
@hfa12 = dso_local global %struct.hfa12 { float 0x4028333340000000, float 0x4028666660000000 }, align 4
@hfa13 = dso_local global %struct.hfa13 { float 0x402A333340000000, float 0x402A666660000000, float 0x402A9999A0000000 }, align 8
@hfa14 = dso_local global %struct.hfa14 { float 0x402C333340000000, float 0x402C666660000000, float 0x402C9999A0000000, float 0x402CCCCCC0000000 }, align 4
@hfa21 = dso_local global %struct.hfa21 { double 2.110000e+01 }, align 8
@hfa22 = dso_local global %struct.hfa22 { double 2.210000e+01, double 2.220000e+01 }, align 8
@hfa23 = dso_local global %struct.hfa23 { double 2.310000e+01, double 2.320000e+01, double 2.330000e+01 }, align 8
@hfa24 = dso_local global %struct.hfa24 { double 2.410000e+01, double 2.420000e+01, double 2.430000e+01, double 2.440000e+01 }, align 8
@hfa31 = dso_local global %struct.hfa31 { x86_fp80 0xK4003F8CCCCCCCCCCD000 }, align 16
@hfa32 = dso_local global %struct.hfa32 { x86_fp80 0xK40048066666666666800, x86_fp80 0xK400480CCCCCCCCCCD000 }, align 16
@hfa33 = dso_local global %struct.hfa33 { x86_fp80 0xK40048466666666666800, x86_fp80 0xK400484CCCCCCCCCCD000, x86_fp80 0xK40048533333333333000 }, align 16
@hfa34 = dso_local global %struct.hfa34 { x86_fp80 0xK40048866666666666800, x86_fp80 0xK400488CCCCCCCCCCD000, x86_fp80 0xK40048933333333333000, x86_fp80 0xK40048999999999999800 }, align 16
@.str = private unnamed_addr constant [6 x i8] c"%.1s\0A\00", align 1
@.str.1 = private unnamed_addr constant [6 x i8] c"%.2s\0A\00", align 1
@.str.2 = private unnamed_addr constant [6 x i8] c"%.3s\0A\00", align 1
@.str.3 = private unnamed_addr constant [6 x i8] c"%.4s\0A\00", align 1
@.str.4 = private unnamed_addr constant [6 x i8] c"%.5s\0A\00", align 1
@.str.5 = private unnamed_addr constant [6 x i8] c"%.6s\0A\00", align 1
@.str.6 = private unnamed_addr constant [6 x i8] c"%.7s\0A\00", align 1
@.str.7 = private unnamed_addr constant [6 x i8] c"%.8s\0A\00", align 1
@.str.8 = private unnamed_addr constant [6 x i8] c"%.9s\0A\00", align 1
@.str.9 = private unnamed_addr constant [7 x i8] c"%.10s\0A\00", align 1
@.str.10 = private unnamed_addr constant [7 x i8] c"%.11s\0A\00", align 1
@.str.11 = private unnamed_addr constant [7 x i8] c"%.12s\0A\00", align 1
@.str.12 = private unnamed_addr constant [7 x i8] c"%.13s\0A\00", align 1
@.str.13 = private unnamed_addr constant [7 x i8] c"%.14s\0A\00", align 1
@.str.14 = private unnamed_addr constant [7 x i8] c"%.15s\0A\00", align 1
@.str.15 = private unnamed_addr constant [7 x i8] c"%.16s\0A\00", align 1
@.str.16 = private unnamed_addr constant [7 x i8] c"%.17s\0A\00", align 1
@.str.17 = private unnamed_addr constant [6 x i8] c"%.1f\0A\00", align 1
@.str.18 = private unnamed_addr constant [11 x i8] c"%.1f %.1f\0A\00", align 1
@.str.19 = private unnamed_addr constant [16 x i8] c"%.1f %.1f %.1f\0A\00", align 1
@.str.20 = private unnamed_addr constant [21 x i8] c"%.1f %.1f %.1f %.1f\0A\00", align 1
@.str.21 = private unnamed_addr constant [7 x i8] c"%.1Lf\0A\00", align 1
@.str.22 = private unnamed_addr constant [13 x i8] c"%.1Lf %.1Lf\0A\00", align 1
@.str.23 = private unnamed_addr constant [19 x i8] c"%.1Lf %.1Lf %.1Lf\0A\00", align 1
@.str.24 = private unnamed_addr constant [25 x i8] c"%.1Lf %.1Lf %.1Lf %.1Lf\0A\00", align 1
@.str.25 = private unnamed_addr constant [31 x i8] c"%.3s %.3s %.3s %.3s %.3s %.3s\0A\00", align 1
@.str.26 = private unnamed_addr constant [33 x i8] c"%.1f %.1f %.1f %.1f %.1Lf %.1Lf\0A\00", align 1
@.str.27 = private unnamed_addr constant [48 x i8] c"%.1s %.1f %.1f %.2s %.1f %.1f %.3s %.1Lf %.1Lf\0A\00", align 1
@.str.28 = private unnamed_addr constant [12 x i8] c"Arguments:\0A\00", align 1
@.str.29 = private unnamed_addr constant [16 x i8] c"Return values:\0A\00", align 1
@.str.30 = private unnamed_addr constant [4 x i8] c"%7s\00", align 1
@.str.31 = private unnamed_addr constant [5 x i8] c"%.7s\00", align 1
@.str.32 = private unnamed_addr constant [4 x i8] c"%9s\00", align 1
@.str.33 = private unnamed_addr constant [5 x i8] c"%.9s\00", align 1
@.str.34 = private unnamed_addr constant [7 x i8] c"%hfa11\00", align 1
@.str.35 = private unnamed_addr constant [10 x i8] c"%.1f,%.1f\00", align 1
@.str.36 = private unnamed_addr constant [7 x i8] c"%hfa12\00", align 1
@.str.37 = private unnamed_addr constant [7 x i8] c"%hfa13\00", align 1
@.str.38 = private unnamed_addr constant [7 x i8] c"%hfa14\00", align 1
@.str.39 = private unnamed_addr constant [7 x i8] c"%hfa21\00", align 1
@.str.40 = private unnamed_addr constant [7 x i8] c"%hfa22\00", align 1
@.str.41 = private unnamed_addr constant [7 x i8] c"%hfa23\00", align 1
@.str.42 = private unnamed_addr constant [7 x i8] c"%hfa24\00", align 1
@.str.43 = private unnamed_addr constant [7 x i8] c"%hfa31\00", align 1
@.str.44 = private unnamed_addr constant [12 x i8] c"%.1Lf,%.1Lf\00", align 1
@.str.45 = private unnamed_addr constant [7 x i8] c"%hfa32\00", align 1
@.str.46 = private unnamed_addr constant [7 x i8] c"%hfa33\00", align 1
@.str.47 = private unnamed_addr constant [7 x i8] c"%hfa34\00", align 1
@.str.48 = private unnamed_addr constant [9 x i8] c"stdarg:\0A\00", align 1
@.str.49 = private unnamed_addr constant [24 x i8] c"%9s %9s %9s %9s %9s %9s\00", align 1
@.str.50 = private unnamed_addr constant [24 x i8] c"%7s %9s %9s %9s %9s %9s\00", align 1
@.str.51 = private unnamed_addr constant [17 x i8] c"HFA long double:\00", align 1
@.str.52 = private unnamed_addr constant [28 x i8] c"%hfa34 %hfa34 %hfa34 %hfa34\00", align 1
@.str.53 = private unnamed_addr constant [28 x i8] c"%hfa33 %hfa34 %hfa34 %hfa34\00", align 1
@.str.54 = private unnamed_addr constant [28 x i8] c"%hfa32 %hfa34 %hfa34 %hfa34\00", align 1
@.str.55 = private unnamed_addr constant [28 x i8] c"%hfa31 %hfa34 %hfa34 %hfa34\00", align 1
@.str.56 = private unnamed_addr constant [35 x i8] c"%hfa32 %hfa33 %hfa33 %hfa33 %hfa33\00", align 1
@.str.57 = private unnamed_addr constant [35 x i8] c"%hfa31 %hfa33 %hfa33 %hfa33 %hfa33\00", align 1
@.str.58 = private unnamed_addr constant [28 x i8] c"%hfa33 %hfa33 %hfa33 %hfa33\00", align 1
@.str.59 = private unnamed_addr constant [35 x i8] c"%hfa34 %hfa32 %hfa32 %hfa32 %hfa32\00", align 1
@.str.60 = private unnamed_addr constant [35 x i8] c"%hfa33 %hfa32 %hfa32 %hfa32 %hfa32\00", align 1
@.str.61 = private unnamed_addr constant [42 x i8] c"%hfa34 %hfa32 %hfa31 %hfa31 %hfa31 %hfa31\00", align 1
@.str.62 = private unnamed_addr constant [12 x i8] c"HFA double:\00", align 1
@.str.63 = private unnamed_addr constant [28 x i8] c"%hfa24 %hfa24 %hfa24 %hfa24\00", align 1
@.str.64 = private unnamed_addr constant [28 x i8] c"%hfa23 %hfa24 %hfa24 %hfa24\00", align 1
@.str.65 = private unnamed_addr constant [28 x i8] c"%hfa22 %hfa24 %hfa24 %hfa24\00", align 1
@.str.66 = private unnamed_addr constant [28 x i8] c"%hfa21 %hfa24 %hfa24 %hfa24\00", align 1
@.str.67 = private unnamed_addr constant [35 x i8] c"%hfa22 %hfa23 %hfa23 %hfa23 %hfa23\00", align 1
@.str.68 = private unnamed_addr constant [35 x i8] c"%hfa21 %hfa23 %hfa23 %hfa23 %hfa23\00", align 1
@.str.69 = private unnamed_addr constant [28 x i8] c"%hfa23 %hfa23 %hfa23 %hfa23\00", align 1
@.str.70 = private unnamed_addr constant [35 x i8] c"%hfa24 %hfa22 %hfa22 %hfa22 %hfa22\00", align 1
@.str.71 = private unnamed_addr constant [35 x i8] c"%hfa23 %hfa22 %hfa22 %hfa22 %hfa22\00", align 1
@.str.72 = private unnamed_addr constant [42 x i8] c"%hfa24 %hfa22 %hfa21 %hfa21 %hfa21 %hfa21\00", align 1
@.str.73 = private unnamed_addr constant [11 x i8] c"HFA float:\00", align 1
@.str.74 = private unnamed_addr constant [28 x i8] c"%hfa14 %hfa14 %hfa14 %hfa14\00", align 1
@.str.75 = private unnamed_addr constant [28 x i8] c"%hfa13 %hfa14 %hfa14 %hfa14\00", align 1
@.str.76 = private unnamed_addr constant [28 x i8] c"%hfa12 %hfa14 %hfa14 %hfa14\00", align 1
@.str.77 = private unnamed_addr constant [28 x i8] c"%hfa11 %hfa14 %hfa14 %hfa14\00", align 1
@.str.78 = private unnamed_addr constant [35 x i8] c"%hfa12 %hfa13 %hfa13 %hfa13 %hfa13\00", align 1
@.str.79 = private unnamed_addr constant [35 x i8] c"%hfa11 %hfa13 %hfa13 %hfa13 %hfa13\00", align 1
@.str.80 = private unnamed_addr constant [28 x i8] c"%hfa13 %hfa13 %hfa13 %hfa13\00", align 1
@.str.81 = private unnamed_addr constant [35 x i8] c"%hfa14 %hfa12 %hfa12 %hfa12 %hfa12\00", align 1
@.str.82 = private unnamed_addr constant [35 x i8] c"%hfa13 %hfa12 %hfa12 %hfa12 %hfa12\00", align 1
@.str.83 = private unnamed_addr constant [42 x i8] c"%hfa14 %hfa12 %hfa11 %hfa11 %hfa11 %hfa11\00", align 1
@.str.84 = private unnamed_addr constant [6 x i8] c"%llx\0A\00", align 1
@.str.85 = private unnamed_addr constant [7 x i8] c"MOVI:\0A\00", align 1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s1(i8 %0) #0 {
  %2 = alloca %struct.s1, align 1
  %3 = getelementptr inbounds %struct.s1, ptr %2, i32 0, i32 0
  store i8 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s1, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [1 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str, ptr noundef %5)
  ret void
}

declare i32 @printf(ptr noundef, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s2(i16 %0) #0 {
  %2 = alloca %struct.s2, align 1
  %3 = getelementptr inbounds %struct.s2, ptr %2, i32 0, i32 0
  store i16 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s2, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [2 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.1, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s3(i24 %0) #0 {
  %2 = alloca %struct.s3, align 1
  %3 = getelementptr inbounds %struct.s3, ptr %2, i32 0, i32 0
  store i24 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s3, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [3 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.2, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s4(i32 %0) #0 {
  %2 = alloca %struct.s4, align 1
  %3 = getelementptr inbounds %struct.s4, ptr %2, i32 0, i32 0
  store i32 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s4, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [4 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.3, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s5(i40 %0) #0 {
  %2 = alloca %struct.s5, align 1
  %3 = getelementptr inbounds %struct.s5, ptr %2, i32 0, i32 0
  store i40 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s5, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [5 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.4, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s6(i48 %0) #0 {
  %2 = alloca %struct.s6, align 1
  %3 = getelementptr inbounds %struct.s6, ptr %2, i32 0, i32 0
  store i48 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s6, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [6 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.5, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s7(i56 %0) #0 {
  %2 = alloca %struct.s7, align 1
  %3 = getelementptr inbounds %struct.s7, ptr %2, i32 0, i32 0
  store i56 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s7, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [7 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.6, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s8(i64 %0) #0 {
  %2 = alloca %struct.s8, align 1
  %3 = getelementptr inbounds %struct.s8, ptr %2, i32 0, i32 0
  store i64 %0, ptr %3, align 1
  %4 = getelementptr inbounds %struct.s8, ptr %2, i32 0, i32 0
  %5 = getelementptr inbounds [8 x i8], ptr %4, i64 0, i64 0
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.7, ptr noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s9(i64 %0, i8 %1) #0 {
  %3 = alloca %struct.s9, align 1
  %4 = alloca { i64, i8 }, align 1
  %5 = getelementptr inbounds { i64, i8 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i8 }, ptr %4, i32 0, i32 1
  store i8 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 9, i1 false)
  %7 = getelementptr inbounds %struct.s9, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [9 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.8, ptr noundef %8)
  ret void
}

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s10(i64 %0, i16 %1) #0 {
  %3 = alloca %struct.s10, align 1
  %4 = alloca { i64, i16 }, align 1
  %5 = getelementptr inbounds { i64, i16 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i16 }, ptr %4, i32 0, i32 1
  store i16 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 10, i1 false)
  %7 = getelementptr inbounds %struct.s10, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [10 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.9, ptr noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s11(i64 %0, i24 %1) #0 {
  %3 = alloca %struct.s11, align 1
  %4 = alloca { i64, i24 }, align 1
  %5 = getelementptr inbounds { i64, i24 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i24 }, ptr %4, i32 0, i32 1
  store i24 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 11, i1 false)
  %7 = getelementptr inbounds %struct.s11, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [11 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.10, ptr noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s12(i64 %0, i32 %1) #0 {
  %3 = alloca %struct.s12, align 1
  %4 = alloca { i64, i32 }, align 1
  %5 = getelementptr inbounds { i64, i32 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i32 }, ptr %4, i32 0, i32 1
  store i32 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 12, i1 false)
  %7 = getelementptr inbounds %struct.s12, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [12 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.11, ptr noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s13(i64 %0, i40 %1) #0 {
  %3 = alloca %struct.s13, align 1
  %4 = alloca { i64, i40 }, align 1
  %5 = getelementptr inbounds { i64, i40 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i40 }, ptr %4, i32 0, i32 1
  store i40 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 13, i1 false)
  %7 = getelementptr inbounds %struct.s13, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [13 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.12, ptr noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s14(i64 %0, i48 %1) #0 {
  %3 = alloca %struct.s14, align 1
  %4 = alloca { i64, i48 }, align 1
  %5 = getelementptr inbounds { i64, i48 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i48 }, ptr %4, i32 0, i32 1
  store i48 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 14, i1 false)
  %7 = getelementptr inbounds %struct.s14, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [14 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.13, ptr noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s15(i64 %0, i56 %1) #0 {
  %3 = alloca %struct.s15, align 1
  %4 = alloca { i64, i56 }, align 1
  %5 = getelementptr inbounds { i64, i56 }, ptr %4, i32 0, i32 0
  store i64 %0, ptr %5, align 1
  %6 = getelementptr inbounds { i64, i56 }, ptr %4, i32 0, i32 1
  store i56 %1, ptr %6, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %3, ptr align 1 %4, i64 15, i1 false)
  %7 = getelementptr inbounds %struct.s15, ptr %3, i32 0, i32 0
  %8 = getelementptr inbounds [15 x i8], ptr %7, i64 0, i64 0
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.14, ptr noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s16(i64 %0, i64 %1) #0 {
  %3 = alloca %struct.s16, align 1
  %4 = getelementptr inbounds { i64, i64 }, ptr %3, i32 0, i32 0
  store i64 %0, ptr %4, align 1
  %5 = getelementptr inbounds { i64, i64 }, ptr %3, i32 0, i32 1
  store i64 %1, ptr %5, align 1
  %6 = getelementptr inbounds %struct.s16, ptr %3, i32 0, i32 0
  %7 = getelementptr inbounds [16 x i8], ptr %6, i64 0, i64 0
  %8 = call i32 (ptr, ...) @printf(ptr noundef @.str.15, ptr noundef %7)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_s17(ptr noundef byval(%struct.s17) align 8 %0) #0 {
  %2 = getelementptr inbounds %struct.s17, ptr %0, i32 0, i32 0
  %3 = getelementptr inbounds [17 x i8], ptr %2, i64 0, i64 0
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.16, ptr noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa11(float %0) #0 {
  %2 = alloca %struct.hfa11, align 4
  %3 = getelementptr inbounds %struct.hfa11, ptr %2, i32 0, i32 0
  store float %0, ptr %3, align 4
  %4 = getelementptr inbounds %struct.hfa11, ptr %2, i32 0, i32 0
  %5 = load float, ptr %4, align 4
  %6 = fpext float %5 to double
  %7 = call i32 (ptr, ...) @printf(ptr noundef @.str.17, double noundef %6)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa12(<2 x float> %0) #3 {
  %2 = alloca %struct.hfa12, align 4
  store <2 x float> %0, ptr %2, align 4
  %3 = getelementptr inbounds %struct.hfa12, ptr %2, i32 0, i32 0
  %4 = load float, ptr %3, align 4
  %5 = fpext float %4 to double
  %6 = getelementptr inbounds %struct.hfa12, ptr %2, i32 0, i32 0
  %7 = load float, ptr %6, align 4
  %8 = fpext float %7 to double
  %9 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %5, double noundef %8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa13(<2 x float> %0, float %1) #3 {
  %3 = alloca %struct.hfa13, align 4
  %4 = alloca { <2 x float>, float }, align 4
  %5 = getelementptr inbounds { <2 x float>, float }, ptr %4, i32 0, i32 0
  store <2 x float> %0, ptr %5, align 4
  %6 = getelementptr inbounds { <2 x float>, float }, ptr %4, i32 0, i32 1
  store float %1, ptr %6, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %3, ptr align 4 %4, i64 12, i1 false)
  %7 = getelementptr inbounds %struct.hfa13, ptr %3, i32 0, i32 0
  %8 = load float, ptr %7, align 4
  %9 = fpext float %8 to double
  %10 = getelementptr inbounds %struct.hfa13, ptr %3, i32 0, i32 1
  %11 = load float, ptr %10, align 4
  %12 = fpext float %11 to double
  %13 = getelementptr inbounds %struct.hfa13, ptr %3, i32 0, i32 2
  %14 = load float, ptr %13, align 4
  %15 = fpext float %14 to double
  %16 = call i32 (ptr, ...) @printf(ptr noundef @.str.19, double noundef %9, double noundef %12, double noundef %15)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa14(<2 x float> %0, <2 x float> %1) #3 {
  %3 = alloca %struct.hfa14, align 4
  %4 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %3, i32 0, i32 0
  store <2 x float> %0, ptr %4, align 4
  %5 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %3, i32 0, i32 1
  store <2 x float> %1, ptr %5, align 4
  %6 = getelementptr inbounds %struct.hfa14, ptr %3, i32 0, i32 0
  %7 = load float, ptr %6, align 4
  %8 = fpext float %7 to double
  %9 = getelementptr inbounds %struct.hfa14, ptr %3, i32 0, i32 1
  %10 = load float, ptr %9, align 4
  %11 = fpext float %10 to double
  %12 = getelementptr inbounds %struct.hfa14, ptr %3, i32 0, i32 2
  %13 = load float, ptr %12, align 4
  %14 = fpext float %13 to double
  %15 = getelementptr inbounds %struct.hfa14, ptr %3, i32 0, i32 3
  %16 = load float, ptr %15, align 4
  %17 = fpext float %16 to double
  %18 = call i32 (ptr, ...) @printf(ptr noundef @.str.20, double noundef %8, double noundef %11, double noundef %14, double noundef %17)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa21(double %0) #0 {
  %2 = alloca %struct.hfa21, align 8
  %3 = getelementptr inbounds %struct.hfa21, ptr %2, i32 0, i32 0
  store double %0, ptr %3, align 8
  %4 = getelementptr inbounds %struct.hfa21, ptr %2, i32 0, i32 0
  %5 = load double, ptr %4, align 8
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.17, double noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa22(double %0, double %1) #0 {
  %3 = alloca %struct.hfa22, align 8
  %4 = getelementptr inbounds { double, double }, ptr %3, i32 0, i32 0
  store double %0, ptr %4, align 8
  %5 = getelementptr inbounds { double, double }, ptr %3, i32 0, i32 1
  store double %1, ptr %5, align 8
  %6 = getelementptr inbounds %struct.hfa22, ptr %3, i32 0, i32 0
  %7 = load double, ptr %6, align 8
  %8 = getelementptr inbounds %struct.hfa22, ptr %3, i32 0, i32 0
  %9 = load double, ptr %8, align 8
  %10 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %7, double noundef %9)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa23(ptr noundef byval(%struct.hfa23) align 8 %0) #0 {
  %2 = getelementptr inbounds %struct.hfa23, ptr %0, i32 0, i32 0
  %3 = load double, ptr %2, align 8
  %4 = getelementptr inbounds %struct.hfa23, ptr %0, i32 0, i32 1
  %5 = load double, ptr %4, align 8
  %6 = getelementptr inbounds %struct.hfa23, ptr %0, i32 0, i32 2
  %7 = load double, ptr %6, align 8
  %8 = call i32 (ptr, ...) @printf(ptr noundef @.str.19, double noundef %3, double noundef %5, double noundef %7)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa24(ptr noundef byval(%struct.hfa24) align 8 %0) #0 {
  %2 = getelementptr inbounds %struct.hfa24, ptr %0, i32 0, i32 0
  %3 = load double, ptr %2, align 8
  %4 = getelementptr inbounds %struct.hfa24, ptr %0, i32 0, i32 1
  %5 = load double, ptr %4, align 8
  %6 = getelementptr inbounds %struct.hfa24, ptr %0, i32 0, i32 2
  %7 = load double, ptr %6, align 8
  %8 = getelementptr inbounds %struct.hfa24, ptr %0, i32 0, i32 3
  %9 = load double, ptr %8, align 8
  %10 = call i32 (ptr, ...) @printf(ptr noundef @.str.20, double noundef %3, double noundef %5, double noundef %7, double noundef %9)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa31(ptr noundef byval(%struct.hfa31) align 16 %0) #0 {
  %2 = getelementptr inbounds %struct.hfa31, ptr %0, i32 0, i32 0
  %3 = load x86_fp80, ptr %2, align 16
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.21, x86_fp80 noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa32(ptr noundef byval(%struct.hfa32) align 16 %0) #0 {
  %2 = getelementptr inbounds %struct.hfa32, ptr %0, i32 0, i32 0
  %3 = load x86_fp80, ptr %2, align 16
  %4 = getelementptr inbounds %struct.hfa32, ptr %0, i32 0, i32 0
  %5 = load x86_fp80, ptr %4, align 16
  %6 = call i32 (ptr, ...) @printf(ptr noundef @.str.22, x86_fp80 noundef %3, x86_fp80 noundef %5)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa33(ptr noundef byval(%struct.hfa33) align 16 %0) #0 {
  %2 = getelementptr inbounds %struct.hfa33, ptr %0, i32 0, i32 0
  %3 = load x86_fp80, ptr %2, align 16
  %4 = getelementptr inbounds %struct.hfa33, ptr %0, i32 0, i32 1
  %5 = load x86_fp80, ptr %4, align 16
  %6 = getelementptr inbounds %struct.hfa33, ptr %0, i32 0, i32 2
  %7 = load x86_fp80, ptr %6, align 16
  %8 = call i32 (ptr, ...) @printf(ptr noundef @.str.23, x86_fp80 noundef %3, x86_fp80 noundef %5, x86_fp80 noundef %7)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa_hfa34(ptr noundef byval(%struct.hfa34) align 16 %0) #0 {
  %2 = getelementptr inbounds %struct.hfa34, ptr %0, i32 0, i32 0
  %3 = load x86_fp80, ptr %2, align 16
  %4 = getelementptr inbounds %struct.hfa34, ptr %0, i32 0, i32 1
  %5 = load x86_fp80, ptr %4, align 16
  %6 = getelementptr inbounds %struct.hfa34, ptr %0, i32 0, i32 2
  %7 = load x86_fp80, ptr %6, align 16
  %8 = getelementptr inbounds %struct.hfa34, ptr %0, i32 0, i32 3
  %9 = load x86_fp80, ptr %8, align 16
  %10 = call i32 (ptr, ...) @printf(ptr noundef @.str.24, x86_fp80 noundef %3, x86_fp80 noundef %5, x86_fp80 noundef %7, x86_fp80 noundef %9)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa1(i64 %0, i64 %1, i8 %2, i64 %3, i16 %4, ptr noundef byval(%struct.s11) align 8 %5, ptr noundef byval(%struct.s12) align 8 %6, ptr noundef byval(%struct.s13) align 8 %7) #0 {
  %9 = alloca %struct.s8, align 1
  %10 = alloca %struct.s9, align 1
  %11 = alloca { i64, i8 }, align 1
  %12 = alloca %struct.s10, align 1
  %13 = alloca { i64, i16 }, align 1
  %14 = getelementptr inbounds %struct.s8, ptr %9, i32 0, i32 0
  store i64 %0, ptr %14, align 1
  %15 = getelementptr inbounds { i64, i8 }, ptr %11, i32 0, i32 0
  store i64 %1, ptr %15, align 1
  %16 = getelementptr inbounds { i64, i8 }, ptr %11, i32 0, i32 1
  store i8 %2, ptr %16, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %10, ptr align 1 %11, i64 9, i1 false)
  %17 = getelementptr inbounds { i64, i16 }, ptr %13, i32 0, i32 0
  store i64 %3, ptr %17, align 1
  %18 = getelementptr inbounds { i64, i16 }, ptr %13, i32 0, i32 1
  store i16 %4, ptr %18, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %12, ptr align 1 %13, i64 10, i1 false)
  %19 = getelementptr inbounds %struct.s8, ptr %9, i32 0, i32 0
  %20 = getelementptr inbounds [8 x i8], ptr %19, i64 0, i64 0
  %21 = getelementptr inbounds %struct.s9, ptr %10, i32 0, i32 0
  %22 = getelementptr inbounds [9 x i8], ptr %21, i64 0, i64 0
  %23 = getelementptr inbounds %struct.s10, ptr %12, i32 0, i32 0
  %24 = getelementptr inbounds [10 x i8], ptr %23, i64 0, i64 0
  %25 = getelementptr inbounds %struct.s11, ptr %5, i32 0, i32 0
  %26 = getelementptr inbounds [11 x i8], ptr %25, i64 0, i64 0
  %27 = getelementptr inbounds %struct.s12, ptr %6, i32 0, i32 0
  %28 = getelementptr inbounds [12 x i8], ptr %27, i64 0, i64 0
  %29 = getelementptr inbounds %struct.s13, ptr %7, i32 0, i32 0
  %30 = getelementptr inbounds [13 x i8], ptr %29, i64 0, i64 0
  %31 = call i32 (ptr, ...) @printf(ptr noundef @.str.25, ptr noundef %20, ptr noundef %22, ptr noundef %24, ptr noundef %26, ptr noundef %28, ptr noundef %30)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa2(i64 %0, i8 %1, i64 %2, i16 %3, i64 %4, i24 %5, ptr noundef byval(%struct.s12) align 8 %6, ptr noundef byval(%struct.s13) align 8 %7, ptr noundef byval(%struct.s14) align 8 %8) #0 {
  %10 = alloca %struct.s9, align 1
  %11 = alloca { i64, i8 }, align 1
  %12 = alloca %struct.s10, align 1
  %13 = alloca { i64, i16 }, align 1
  %14 = alloca %struct.s11, align 1
  %15 = alloca { i64, i24 }, align 1
  %16 = getelementptr inbounds { i64, i8 }, ptr %11, i32 0, i32 0
  store i64 %0, ptr %16, align 1
  %17 = getelementptr inbounds { i64, i8 }, ptr %11, i32 0, i32 1
  store i8 %1, ptr %17, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %10, ptr align 1 %11, i64 9, i1 false)
  %18 = getelementptr inbounds { i64, i16 }, ptr %13, i32 0, i32 0
  store i64 %2, ptr %18, align 1
  %19 = getelementptr inbounds { i64, i16 }, ptr %13, i32 0, i32 1
  store i16 %3, ptr %19, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %12, ptr align 1 %13, i64 10, i1 false)
  %20 = getelementptr inbounds { i64, i24 }, ptr %15, i32 0, i32 0
  store i64 %4, ptr %20, align 1
  %21 = getelementptr inbounds { i64, i24 }, ptr %15, i32 0, i32 1
  store i24 %5, ptr %21, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %14, ptr align 1 %15, i64 11, i1 false)
  %22 = getelementptr inbounds %struct.s9, ptr %10, i32 0, i32 0
  %23 = getelementptr inbounds [9 x i8], ptr %22, i64 0, i64 0
  %24 = getelementptr inbounds %struct.s10, ptr %12, i32 0, i32 0
  %25 = getelementptr inbounds [10 x i8], ptr %24, i64 0, i64 0
  %26 = getelementptr inbounds %struct.s11, ptr %14, i32 0, i32 0
  %27 = getelementptr inbounds [11 x i8], ptr %26, i64 0, i64 0
  %28 = getelementptr inbounds %struct.s12, ptr %6, i32 0, i32 0
  %29 = getelementptr inbounds [12 x i8], ptr %28, i64 0, i64 0
  %30 = getelementptr inbounds %struct.s13, ptr %7, i32 0, i32 0
  %31 = getelementptr inbounds [13 x i8], ptr %30, i64 0, i64 0
  %32 = getelementptr inbounds %struct.s14, ptr %8, i32 0, i32 0
  %33 = getelementptr inbounds [14 x i8], ptr %32, i64 0, i64 0
  %34 = call i32 (ptr, ...) @printf(ptr noundef @.str.25, ptr noundef %23, ptr noundef %25, ptr noundef %27, ptr noundef %29, ptr noundef %31, ptr noundef %33)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa3(<2 x float> %0, <2 x float> %1, ptr noundef byval(%struct.hfa23) align 8 %2, ptr noundef byval(%struct.hfa32) align 16 %3) #3 {
  %5 = alloca %struct.hfa14, align 4
  %6 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %5, i32 0, i32 0
  store <2 x float> %0, ptr %6, align 4
  %7 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %5, i32 0, i32 1
  store <2 x float> %1, ptr %7, align 4
  %8 = getelementptr inbounds %struct.hfa14, ptr %5, i32 0, i32 0
  %9 = load float, ptr %8, align 4
  %10 = fpext float %9 to double
  %11 = getelementptr inbounds %struct.hfa14, ptr %5, i32 0, i32 3
  %12 = load float, ptr %11, align 4
  %13 = fpext float %12 to double
  %14 = getelementptr inbounds %struct.hfa23, ptr %2, i32 0, i32 0
  %15 = load double, ptr %14, align 8
  %16 = getelementptr inbounds %struct.hfa23, ptr %2, i32 0, i32 2
  %17 = load double, ptr %16, align 8
  %18 = getelementptr inbounds %struct.hfa32, ptr %3, i32 0, i32 0
  %19 = load x86_fp80, ptr %18, align 16
  %20 = getelementptr inbounds %struct.hfa32, ptr %3, i32 0, i32 1
  %21 = load x86_fp80, ptr %20, align 16
  %22 = call i32 (ptr, ...) @printf(ptr noundef @.str.26, double noundef %10, double noundef %13, double noundef %15, double noundef %17, x86_fp80 noundef %19, x86_fp80 noundef %21)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fa4(i8 %0, <2 x float> %1, <2 x float> %2, i16 %3, ptr noundef byval(%struct.hfa24) align 8 %4, i24 %5, ptr noundef byval(%struct.hfa34) align 16 %6) #3 {
  %8 = alloca %struct.s1, align 1
  %9 = alloca %struct.hfa14, align 4
  %10 = alloca %struct.s2, align 1
  %11 = alloca %struct.s3, align 1
  %12 = getelementptr inbounds %struct.s1, ptr %8, i32 0, i32 0
  store i8 %0, ptr %12, align 1
  %13 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %9, i32 0, i32 0
  store <2 x float> %1, ptr %13, align 4
  %14 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %9, i32 0, i32 1
  store <2 x float> %2, ptr %14, align 4
  %15 = getelementptr inbounds %struct.s2, ptr %10, i32 0, i32 0
  store i16 %3, ptr %15, align 1
  %16 = getelementptr inbounds %struct.s3, ptr %11, i32 0, i32 0
  store i24 %5, ptr %16, align 1
  %17 = getelementptr inbounds %struct.s1, ptr %8, i32 0, i32 0
  %18 = getelementptr inbounds [1 x i8], ptr %17, i64 0, i64 0
  %19 = getelementptr inbounds %struct.hfa14, ptr %9, i32 0, i32 0
  %20 = load float, ptr %19, align 4
  %21 = fpext float %20 to double
  %22 = getelementptr inbounds %struct.hfa14, ptr %9, i32 0, i32 3
  %23 = load float, ptr %22, align 4
  %24 = fpext float %23 to double
  %25 = getelementptr inbounds %struct.s2, ptr %10, i32 0, i32 0
  %26 = getelementptr inbounds [2 x i8], ptr %25, i64 0, i64 0
  %27 = getelementptr inbounds %struct.hfa24, ptr %4, i32 0, i32 0
  %28 = load double, ptr %27, align 8
  %29 = getelementptr inbounds %struct.hfa24, ptr %4, i32 0, i32 3
  %30 = load double, ptr %29, align 8
  %31 = getelementptr inbounds %struct.s3, ptr %11, i32 0, i32 0
  %32 = getelementptr inbounds [3 x i8], ptr %31, i64 0, i64 0
  %33 = getelementptr inbounds %struct.hfa34, ptr %6, i32 0, i32 0
  %34 = load x86_fp80, ptr %33, align 16
  %35 = getelementptr inbounds %struct.hfa34, ptr %6, i32 0, i32 3
  %36 = load x86_fp80, ptr %35, align 16
  %37 = call i32 (ptr, ...) @printf(ptr noundef @.str.27, ptr noundef %18, double noundef %21, double noundef %24, ptr noundef %26, double noundef %28, double noundef %30, ptr noundef %32, x86_fp80 noundef %34, x86_fp80 noundef %36)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @arg() #3 {
  %1 = alloca i24, align 4
  %2 = alloca i40, align 8
  %3 = alloca i48, align 8
  %4 = alloca i56, align 8
  %5 = alloca { i64, i8 }, align 1
  %6 = alloca { i64, i16 }, align 1
  %7 = alloca { i64, i24 }, align 1
  %8 = alloca { i64, i32 }, align 1
  %9 = alloca { i64, i40 }, align 1
  %10 = alloca { i64, i48 }, align 1
  %11 = alloca { i64, i56 }, align 1
  %12 = alloca { <2 x float>, float }, align 4
  %13 = alloca { i64, i8 }, align 1
  %14 = alloca { i64, i16 }, align 1
  %15 = alloca { i64, i8 }, align 1
  %16 = alloca { i64, i16 }, align 1
  %17 = alloca { i64, i24 }, align 1
  %18 = alloca i24, align 4
  %19 = call i32 (ptr, ...) @printf(ptr noundef @.str.28)
  %20 = load i8, ptr @s1, align 1
  call void @fa_s1(i8 %20)
  %21 = load i16, ptr @s2, align 1
  call void @fa_s2(i16 %21)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %1, ptr align 1 @s3, i64 3, i1 false)
  %22 = load i24, ptr %1, align 4
  call void @fa_s3(i24 %22)
  %23 = load i32, ptr @s4, align 1
  call void @fa_s4(i32 %23)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 @s5, i64 5, i1 false)
  %24 = load i40, ptr %2, align 8
  call void @fa_s5(i40 %24)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %3, ptr align 1 @s6, i64 6, i1 false)
  %25 = load i48, ptr %3, align 8
  call void @fa_s6(i48 %25)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %4, ptr align 1 @s7, i64 7, i1 false)
  %26 = load i56, ptr %4, align 8
  call void @fa_s7(i56 %26)
  %27 = load i64, ptr @s8, align 1
  call void @fa_s8(i64 %27)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %5, ptr align 1 @s9, i64 9, i1 false)
  %28 = getelementptr inbounds { i64, i8 }, ptr %5, i32 0, i32 0
  %29 = load i64, ptr %28, align 1
  %30 = getelementptr inbounds { i64, i8 }, ptr %5, i32 0, i32 1
  %31 = load i8, ptr %30, align 1
  call void @fa_s9(i64 %29, i8 %31)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %6, ptr align 1 @s10, i64 10, i1 false)
  %32 = getelementptr inbounds { i64, i16 }, ptr %6, i32 0, i32 0
  %33 = load i64, ptr %32, align 1
  %34 = getelementptr inbounds { i64, i16 }, ptr %6, i32 0, i32 1
  %35 = load i16, ptr %34, align 1
  call void @fa_s10(i64 %33, i16 %35)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %7, ptr align 1 @s11, i64 11, i1 false)
  %36 = getelementptr inbounds { i64, i24 }, ptr %7, i32 0, i32 0
  %37 = load i64, ptr %36, align 1
  %38 = getelementptr inbounds { i64, i24 }, ptr %7, i32 0, i32 1
  %39 = load i24, ptr %38, align 1
  call void @fa_s11(i64 %37, i24 %39)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %8, ptr align 1 @s12, i64 12, i1 false)
  %40 = getelementptr inbounds { i64, i32 }, ptr %8, i32 0, i32 0
  %41 = load i64, ptr %40, align 1
  %42 = getelementptr inbounds { i64, i32 }, ptr %8, i32 0, i32 1
  %43 = load i32, ptr %42, align 1
  call void @fa_s12(i64 %41, i32 %43)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %9, ptr align 1 @s13, i64 13, i1 false)
  %44 = getelementptr inbounds { i64, i40 }, ptr %9, i32 0, i32 0
  %45 = load i64, ptr %44, align 1
  %46 = getelementptr inbounds { i64, i40 }, ptr %9, i32 0, i32 1
  %47 = load i40, ptr %46, align 1
  call void @fa_s13(i64 %45, i40 %47)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %10, ptr align 1 @s14, i64 14, i1 false)
  %48 = getelementptr inbounds { i64, i48 }, ptr %10, i32 0, i32 0
  %49 = load i64, ptr %48, align 1
  %50 = getelementptr inbounds { i64, i48 }, ptr %10, i32 0, i32 1
  %51 = load i48, ptr %50, align 1
  call void @fa_s14(i64 %49, i48 %51)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %11, ptr align 1 @s15, i64 15, i1 false)
  %52 = getelementptr inbounds { i64, i56 }, ptr %11, i32 0, i32 0
  %53 = load i64, ptr %52, align 1
  %54 = getelementptr inbounds { i64, i56 }, ptr %11, i32 0, i32 1
  %55 = load i56, ptr %54, align 1
  call void @fa_s15(i64 %53, i56 %55)
  %56 = load i64, ptr @s16, align 1
  %57 = load i64, ptr getelementptr inbounds ({ i64, i64 }, ptr @s16, i32 0, i32 1), align 1
  call void @fa_s16(i64 %56, i64 %57)
  call void @fa_s17(ptr noundef byval(%struct.s17) align 8 @s17)
  %58 = load float, ptr @hfa11, align 4
  call void @fa_hfa11(float %58)
  %59 = load <2 x float>, ptr @hfa12, align 4
  call void @fa_hfa12(<2 x float> %59)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %12, ptr align 4 @hfa13, i64 12, i1 false)
  %60 = getelementptr inbounds { <2 x float>, float }, ptr %12, i32 0, i32 0
  %61 = load <2 x float>, ptr %60, align 4
  %62 = getelementptr inbounds { <2 x float>, float }, ptr %12, i32 0, i32 1
  %63 = load float, ptr %62, align 4
  call void @fa_hfa13(<2 x float> %61, float %63)
  %64 = load <2 x float>, ptr @hfa14, align 4
  %65 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  call void @fa_hfa14(<2 x float> %64, <2 x float> %65)
  %66 = load double, ptr @hfa21, align 8
  call void @fa_hfa21(double %66)
  %67 = load double, ptr @hfa22, align 8
  %68 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  call void @fa_hfa22(double %67, double %68)
  call void @fa_hfa23(ptr noundef byval(%struct.hfa23) align 8 @hfa23)
  call void @fa_hfa24(ptr noundef byval(%struct.hfa24) align 8 @hfa24)
  call void @fa_hfa31(ptr noundef byval(%struct.hfa31) align 16 @hfa31)
  call void @fa_hfa32(ptr noundef byval(%struct.hfa32) align 16 @hfa32)
  call void @fa_hfa33(ptr noundef byval(%struct.hfa33) align 16 @hfa33)
  call void @fa_hfa34(ptr noundef byval(%struct.hfa34) align 16 @hfa34)
  %69 = load i64, ptr @s8, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %13, ptr align 1 @s9, i64 9, i1 false)
  %70 = getelementptr inbounds { i64, i8 }, ptr %13, i32 0, i32 0
  %71 = load i64, ptr %70, align 1
  %72 = getelementptr inbounds { i64, i8 }, ptr %13, i32 0, i32 1
  %73 = load i8, ptr %72, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %14, ptr align 1 @s10, i64 10, i1 false)
  %74 = getelementptr inbounds { i64, i16 }, ptr %14, i32 0, i32 0
  %75 = load i64, ptr %74, align 1
  %76 = getelementptr inbounds { i64, i16 }, ptr %14, i32 0, i32 1
  %77 = load i16, ptr %76, align 1
  call void @fa1(i64 %69, i64 %71, i8 %73, i64 %75, i16 %77, ptr noundef byval(%struct.s11) align 8 @s11, ptr noundef byval(%struct.s12) align 8 @s12, ptr noundef byval(%struct.s13) align 8 @s13)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %15, ptr align 1 @s9, i64 9, i1 false)
  %78 = getelementptr inbounds { i64, i8 }, ptr %15, i32 0, i32 0
  %79 = load i64, ptr %78, align 1
  %80 = getelementptr inbounds { i64, i8 }, ptr %15, i32 0, i32 1
  %81 = load i8, ptr %80, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %16, ptr align 1 @s10, i64 10, i1 false)
  %82 = getelementptr inbounds { i64, i16 }, ptr %16, i32 0, i32 0
  %83 = load i64, ptr %82, align 1
  %84 = getelementptr inbounds { i64, i16 }, ptr %16, i32 0, i32 1
  %85 = load i16, ptr %84, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %17, ptr align 1 @s11, i64 11, i1 false)
  %86 = getelementptr inbounds { i64, i24 }, ptr %17, i32 0, i32 0
  %87 = load i64, ptr %86, align 1
  %88 = getelementptr inbounds { i64, i24 }, ptr %17, i32 0, i32 1
  %89 = load i24, ptr %88, align 1
  call void @fa2(i64 %79, i8 %81, i64 %83, i16 %85, i64 %87, i24 %89, ptr noundef byval(%struct.s12) align 8 @s12, ptr noundef byval(%struct.s13) align 8 @s13, ptr noundef byval(%struct.s14) align 8 @s14)
  %90 = load <2 x float>, ptr @hfa14, align 4
  %91 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  call void @fa3(<2 x float> %90, <2 x float> %91, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa32) align 16 @hfa32)
  %92 = load i8, ptr @s1, align 1
  %93 = load <2 x float>, ptr @hfa14, align 4
  %94 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %95 = load i16, ptr @s2, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %18, ptr align 1 @s3, i64 3, i1 false)
  %96 = load i24, ptr %18, align 4
  call void @fa4(i8 %92, <2 x float> %93, <2 x float> %94, i16 %95, ptr noundef byval(%struct.hfa24) align 8 @hfa24, i24 %96, ptr noundef byval(%struct.hfa34) align 16 @hfa34)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i8 @fr_s1() #0 {
  %1 = alloca %struct.s1, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s1, i64 1, i1 false)
  %2 = getelementptr inbounds %struct.s1, ptr %1, i32 0, i32 0
  %3 = load i8, ptr %2, align 1
  ret i8 %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i16 @fr_s2() #0 {
  %1 = alloca %struct.s2, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s2, i64 2, i1 false)
  %2 = getelementptr inbounds %struct.s2, ptr %1, i32 0, i32 0
  %3 = load i16, ptr %2, align 1
  ret i16 %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i24 @fr_s3() #0 {
  %1 = alloca %struct.s3, align 1
  %2 = alloca i24, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s3, i64 3, i1 false)
  %3 = getelementptr inbounds %struct.s3, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %2, ptr align 1 %3, i64 3, i1 false)
  %4 = load i24, ptr %2, align 4
  ret i24 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @fr_s4() #0 {
  %1 = alloca %struct.s4, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s4, i64 4, i1 false)
  %2 = getelementptr inbounds %struct.s4, ptr %1, i32 0, i32 0
  %3 = load i32, ptr %2, align 1
  ret i32 %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i40 @fr_s5() #0 {
  %1 = alloca %struct.s5, align 1
  %2 = alloca i40, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s5, i64 5, i1 false)
  %3 = getelementptr inbounds %struct.s5, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 5, i1 false)
  %4 = load i40, ptr %2, align 8
  ret i40 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i48 @fr_s6() #0 {
  %1 = alloca %struct.s6, align 1
  %2 = alloca i48, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s6, i64 6, i1 false)
  %3 = getelementptr inbounds %struct.s6, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 6, i1 false)
  %4 = load i48, ptr %2, align 8
  ret i48 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i56 @fr_s7() #0 {
  %1 = alloca %struct.s7, align 1
  %2 = alloca i56, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s7, i64 7, i1 false)
  %3 = getelementptr inbounds %struct.s7, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 7, i1 false)
  %4 = load i56, ptr %2, align 8
  ret i56 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i64 @fr_s8() #0 {
  %1 = alloca %struct.s8, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s8, i64 8, i1 false)
  %2 = getelementptr inbounds %struct.s8, ptr %1, i32 0, i32 0
  %3 = load i64, ptr %2, align 1
  ret i64 %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i8 } @fr_s9() #0 {
  %1 = alloca %struct.s9, align 1
  %2 = alloca { i64, i8 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s9, i64 9, i1 false)
  %3 = getelementptr inbounds %struct.s9, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 9, i1 false)
  %4 = load { i64, i8 }, ptr %2, align 8
  ret { i64, i8 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i16 } @fr_s10() #0 {
  %1 = alloca %struct.s10, align 1
  %2 = alloca { i64, i16 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s10, i64 10, i1 false)
  %3 = getelementptr inbounds %struct.s10, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 10, i1 false)
  %4 = load { i64, i16 }, ptr %2, align 8
  ret { i64, i16 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i24 } @fr_s11() #0 {
  %1 = alloca %struct.s11, align 1
  %2 = alloca { i64, i24 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s11, i64 11, i1 false)
  %3 = getelementptr inbounds %struct.s11, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 11, i1 false)
  %4 = load { i64, i24 }, ptr %2, align 8
  ret { i64, i24 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i32 } @fr_s12() #0 {
  %1 = alloca %struct.s12, align 1
  %2 = alloca { i64, i32 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s12, i64 12, i1 false)
  %3 = getelementptr inbounds %struct.s12, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 12, i1 false)
  %4 = load { i64, i32 }, ptr %2, align 8
  ret { i64, i32 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i40 } @fr_s13() #0 {
  %1 = alloca %struct.s13, align 1
  %2 = alloca { i64, i40 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s13, i64 13, i1 false)
  %3 = getelementptr inbounds %struct.s13, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 13, i1 false)
  %4 = load { i64, i40 }, ptr %2, align 8
  ret { i64, i40 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i48 } @fr_s14() #0 {
  %1 = alloca %struct.s14, align 1
  %2 = alloca { i64, i48 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s14, i64 14, i1 false)
  %3 = getelementptr inbounds %struct.s14, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 14, i1 false)
  %4 = load { i64, i48 }, ptr %2, align 8
  ret { i64, i48 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i56 } @fr_s15() #0 {
  %1 = alloca %struct.s15, align 1
  %2 = alloca { i64, i56 }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s15, i64 15, i1 false)
  %3 = getelementptr inbounds %struct.s15, ptr %1, i32 0, i32 0
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 1 %3, i64 15, i1 false)
  %4 = load { i64, i56 }, ptr %2, align 8
  ret { i64, i56 } %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { i64, i64 } @fr_s16() #0 {
  %1 = alloca %struct.s16, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s16, i64 16, i1 false)
  %2 = getelementptr inbounds %struct.s16, ptr %1, i32 0, i32 0
  %3 = load { i64, i64 }, ptr %2, align 1
  ret { i64, i64 } %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fr_s17(ptr dead_on_unwind noalias writable sret(%struct.s17) align 1 %0) #0 {
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %0, ptr align 1 @s17, i64 17, i1 false)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local float @fr_hfa11() #0 {
  %1 = alloca %struct.hfa11, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %1, ptr align 4 @hfa11, i64 4, i1 false)
  %2 = getelementptr inbounds %struct.hfa11, ptr %1, i32 0, i32 0
  %3 = load float, ptr %2, align 4
  ret float %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local <2 x float> @fr_hfa12() #3 {
  %1 = alloca %struct.hfa12, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %1, ptr align 4 @hfa12, i64 8, i1 false)
  %2 = load <2 x float>, ptr %1, align 4
  ret <2 x float> %2
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { <2 x float>, float } @fr_hfa13() #0 {
  %1 = alloca %struct.hfa13, align 4
  %2 = alloca { <2 x float>, float }, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %1, ptr align 4 @hfa13, i64 12, i1 false)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %2, ptr align 4 %1, i64 12, i1 false)
  %3 = load { <2 x float>, float }, ptr %2, align 8
  ret { <2 x float>, float } %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { <2 x float>, <2 x float> } @fr_hfa14() #0 {
  %1 = alloca %struct.hfa14, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %1, ptr align 4 @hfa14, i64 16, i1 false)
  %2 = load { <2 x float>, <2 x float> }, ptr %1, align 4
  ret { <2 x float>, <2 x float> } %2
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local double @fr_hfa21() #0 {
  %1 = alloca %struct.hfa21, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %1, ptr align 8 @hfa21, i64 8, i1 false)
  %2 = getelementptr inbounds %struct.hfa21, ptr %1, i32 0, i32 0
  %3 = load double, ptr %2, align 8
  ret double %3
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local { double, double } @fr_hfa22() #0 {
  %1 = alloca %struct.hfa22, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %1, ptr align 8 @hfa22, i64 16, i1 false)
  %2 = load { double, double }, ptr %1, align 8
  ret { double, double } %2
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fr_hfa23(ptr dead_on_unwind noalias writable sret(%struct.hfa23) align 8 %0) #0 {
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %0, ptr align 8 @hfa23, i64 24, i1 false)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fr_hfa24(ptr dead_on_unwind noalias writable sret(%struct.hfa24) align 8 %0) #0 {
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %0, ptr align 8 @hfa24, i64 32, i1 false)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local x86_fp80 @fr_hfa31() #0 {
  %1 = alloca %struct.hfa31, align 16
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %1, ptr align 16 @hfa31, i64 16, i1 false)
  %2 = load x86_fp80, ptr %1, align 16
  ret x86_fp80 %2
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fr_hfa32(ptr dead_on_unwind noalias writable sret(%struct.hfa32) align 16 %0) #0 {
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %0, ptr align 16 @hfa32, i64 32, i1 false)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fr_hfa33(ptr dead_on_unwind noalias writable sret(%struct.hfa33) align 16 %0) #0 {
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %0, ptr align 16 @hfa33, i64 48, i1 false)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @fr_hfa34(ptr dead_on_unwind noalias writable sret(%struct.hfa34) align 16 %0) #0 {
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %0, ptr align 16 @hfa34, i64 64, i1 false)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @ret() #3 {
  %1 = alloca %struct.s1, align 1
  %2 = alloca %struct.s2, align 1
  %3 = alloca %struct.s3, align 1
  %4 = alloca %struct.s4, align 1
  %5 = alloca %struct.s5, align 1
  %6 = alloca %struct.s6, align 1
  %7 = alloca %struct.s7, align 1
  %8 = alloca %struct.s8, align 1
  %9 = alloca %struct.s9, align 1
  %10 = alloca { i64, i8 }, align 8
  %11 = alloca %struct.s10, align 1
  %12 = alloca { i64, i16 }, align 8
  %13 = alloca %struct.s11, align 1
  %14 = alloca { i64, i24 }, align 8
  %15 = alloca %struct.s12, align 1
  %16 = alloca { i64, i32 }, align 8
  %17 = alloca %struct.s13, align 1
  %18 = alloca { i64, i40 }, align 8
  %19 = alloca %struct.s14, align 1
  %20 = alloca { i64, i48 }, align 8
  %21 = alloca %struct.s15, align 1
  %22 = alloca { i64, i56 }, align 8
  %23 = alloca %struct.s16, align 1
  %24 = alloca %struct.s17, align 1
  %25 = alloca %struct.hfa11, align 4
  %26 = alloca %struct.hfa12, align 4
  %27 = alloca %struct.hfa12, align 4
  %28 = alloca %struct.hfa13, align 4
  %29 = alloca { <2 x float>, float }, align 8
  %30 = alloca %struct.hfa13, align 4
  %31 = alloca { <2 x float>, float }, align 8
  %32 = alloca %struct.hfa14, align 4
  %33 = alloca %struct.hfa14, align 4
  %34 = alloca %struct.hfa21, align 8
  %35 = alloca %struct.hfa22, align 8
  %36 = alloca %struct.hfa22, align 8
  %37 = alloca %struct.hfa23, align 8
  %38 = alloca %struct.hfa23, align 8
  %39 = alloca %struct.hfa24, align 8
  %40 = alloca %struct.hfa24, align 8
  %41 = alloca %struct.hfa31, align 16
  %42 = alloca %struct.hfa32, align 16
  %43 = alloca %struct.hfa32, align 16
  %44 = alloca %struct.hfa33, align 16
  %45 = alloca %struct.hfa33, align 16
  %46 = alloca %struct.hfa34, align 16
  %47 = alloca %struct.hfa34, align 16
  %48 = call i8 @fr_s1()
  %49 = getelementptr inbounds %struct.s1, ptr %1, i32 0, i32 0
  store i8 %48, ptr %49, align 1
  %50 = call i16 @fr_s2()
  %51 = getelementptr inbounds %struct.s2, ptr %2, i32 0, i32 0
  store i16 %50, ptr %51, align 1
  %52 = call i24 @fr_s3()
  %53 = getelementptr inbounds %struct.s3, ptr %3, i32 0, i32 0
  store i24 %52, ptr %53, align 1
  %54 = call i32 @fr_s4()
  %55 = getelementptr inbounds %struct.s4, ptr %4, i32 0, i32 0
  store i32 %54, ptr %55, align 1
  %56 = call i40 @fr_s5()
  %57 = getelementptr inbounds %struct.s5, ptr %5, i32 0, i32 0
  store i40 %56, ptr %57, align 1
  %58 = call i48 @fr_s6()
  %59 = getelementptr inbounds %struct.s6, ptr %6, i32 0, i32 0
  store i48 %58, ptr %59, align 1
  %60 = call i56 @fr_s7()
  %61 = getelementptr inbounds %struct.s7, ptr %7, i32 0, i32 0
  store i56 %60, ptr %61, align 1
  %62 = call i64 @fr_s8()
  %63 = getelementptr inbounds %struct.s8, ptr %8, i32 0, i32 0
  store i64 %62, ptr %63, align 1
  %64 = call { i64, i8 } @fr_s9()
  %65 = getelementptr inbounds %struct.s9, ptr %9, i32 0, i32 0
  store { i64, i8 } %64, ptr %10, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %65, ptr align 8 %10, i64 9, i1 false)
  %66 = call { i64, i16 } @fr_s10()
  %67 = getelementptr inbounds %struct.s10, ptr %11, i32 0, i32 0
  store { i64, i16 } %66, ptr %12, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %67, ptr align 8 %12, i64 10, i1 false)
  %68 = call { i64, i24 } @fr_s11()
  %69 = getelementptr inbounds %struct.s11, ptr %13, i32 0, i32 0
  store { i64, i24 } %68, ptr %14, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %69, ptr align 8 %14, i64 11, i1 false)
  %70 = call { i64, i32 } @fr_s12()
  %71 = getelementptr inbounds %struct.s12, ptr %15, i32 0, i32 0
  store { i64, i32 } %70, ptr %16, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %71, ptr align 8 %16, i64 12, i1 false)
  %72 = call { i64, i40 } @fr_s13()
  %73 = getelementptr inbounds %struct.s13, ptr %17, i32 0, i32 0
  store { i64, i40 } %72, ptr %18, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %73, ptr align 8 %18, i64 13, i1 false)
  %74 = call { i64, i48 } @fr_s14()
  %75 = getelementptr inbounds %struct.s14, ptr %19, i32 0, i32 0
  store { i64, i48 } %74, ptr %20, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %75, ptr align 8 %20, i64 14, i1 false)
  %76 = call { i64, i56 } @fr_s15()
  %77 = getelementptr inbounds %struct.s15, ptr %21, i32 0, i32 0
  store { i64, i56 } %76, ptr %22, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %77, ptr align 8 %22, i64 15, i1 false)
  %78 = call { i64, i64 } @fr_s16()
  %79 = getelementptr inbounds %struct.s16, ptr %23, i32 0, i32 0
  %80 = getelementptr inbounds { i64, i64 }, ptr %79, i32 0, i32 0
  %81 = extractvalue { i64, i64 } %78, 0
  store i64 %81, ptr %80, align 1
  %82 = getelementptr inbounds { i64, i64 }, ptr %79, i32 0, i32 1
  %83 = extractvalue { i64, i64 } %78, 1
  store i64 %83, ptr %82, align 1
  call void @fr_s17(ptr dead_on_unwind writable sret(%struct.s17) align 1 %24)
  %84 = call i32 (ptr, ...) @printf(ptr noundef @.str.29)
  %85 = getelementptr inbounds %struct.s1, ptr %1, i32 0, i32 0
  %86 = getelementptr inbounds [1 x i8], ptr %85, i64 0, i64 0
  %87 = call i32 (ptr, ...) @printf(ptr noundef @.str, ptr noundef %86)
  %88 = getelementptr inbounds %struct.s2, ptr %2, i32 0, i32 0
  %89 = getelementptr inbounds [2 x i8], ptr %88, i64 0, i64 0
  %90 = call i32 (ptr, ...) @printf(ptr noundef @.str.1, ptr noundef %89)
  %91 = getelementptr inbounds %struct.s3, ptr %3, i32 0, i32 0
  %92 = getelementptr inbounds [3 x i8], ptr %91, i64 0, i64 0
  %93 = call i32 (ptr, ...) @printf(ptr noundef @.str.2, ptr noundef %92)
  %94 = getelementptr inbounds %struct.s4, ptr %4, i32 0, i32 0
  %95 = getelementptr inbounds [4 x i8], ptr %94, i64 0, i64 0
  %96 = call i32 (ptr, ...) @printf(ptr noundef @.str.3, ptr noundef %95)
  %97 = getelementptr inbounds %struct.s5, ptr %5, i32 0, i32 0
  %98 = getelementptr inbounds [5 x i8], ptr %97, i64 0, i64 0
  %99 = call i32 (ptr, ...) @printf(ptr noundef @.str.4, ptr noundef %98)
  %100 = getelementptr inbounds %struct.s6, ptr %6, i32 0, i32 0
  %101 = getelementptr inbounds [6 x i8], ptr %100, i64 0, i64 0
  %102 = call i32 (ptr, ...) @printf(ptr noundef @.str.5, ptr noundef %101)
  %103 = getelementptr inbounds %struct.s7, ptr %7, i32 0, i32 0
  %104 = getelementptr inbounds [7 x i8], ptr %103, i64 0, i64 0
  %105 = call i32 (ptr, ...) @printf(ptr noundef @.str.6, ptr noundef %104)
  %106 = getelementptr inbounds %struct.s8, ptr %8, i32 0, i32 0
  %107 = getelementptr inbounds [8 x i8], ptr %106, i64 0, i64 0
  %108 = call i32 (ptr, ...) @printf(ptr noundef @.str.7, ptr noundef %107)
  %109 = getelementptr inbounds %struct.s9, ptr %9, i32 0, i32 0
  %110 = getelementptr inbounds [9 x i8], ptr %109, i64 0, i64 0
  %111 = call i32 (ptr, ...) @printf(ptr noundef @.str.8, ptr noundef %110)
  %112 = getelementptr inbounds %struct.s10, ptr %11, i32 0, i32 0
  %113 = getelementptr inbounds [10 x i8], ptr %112, i64 0, i64 0
  %114 = call i32 (ptr, ...) @printf(ptr noundef @.str.9, ptr noundef %113)
  %115 = getelementptr inbounds %struct.s11, ptr %13, i32 0, i32 0
  %116 = getelementptr inbounds [11 x i8], ptr %115, i64 0, i64 0
  %117 = call i32 (ptr, ...) @printf(ptr noundef @.str.10, ptr noundef %116)
  %118 = getelementptr inbounds %struct.s12, ptr %15, i32 0, i32 0
  %119 = getelementptr inbounds [12 x i8], ptr %118, i64 0, i64 0
  %120 = call i32 (ptr, ...) @printf(ptr noundef @.str.11, ptr noundef %119)
  %121 = getelementptr inbounds %struct.s13, ptr %17, i32 0, i32 0
  %122 = getelementptr inbounds [13 x i8], ptr %121, i64 0, i64 0
  %123 = call i32 (ptr, ...) @printf(ptr noundef @.str.12, ptr noundef %122)
  %124 = getelementptr inbounds %struct.s14, ptr %19, i32 0, i32 0
  %125 = getelementptr inbounds [14 x i8], ptr %124, i64 0, i64 0
  %126 = call i32 (ptr, ...) @printf(ptr noundef @.str.13, ptr noundef %125)
  %127 = getelementptr inbounds %struct.s15, ptr %21, i32 0, i32 0
  %128 = getelementptr inbounds [15 x i8], ptr %127, i64 0, i64 0
  %129 = call i32 (ptr, ...) @printf(ptr noundef @.str.14, ptr noundef %128)
  %130 = getelementptr inbounds %struct.s16, ptr %23, i32 0, i32 0
  %131 = getelementptr inbounds [16 x i8], ptr %130, i64 0, i64 0
  %132 = call i32 (ptr, ...) @printf(ptr noundef @.str.15, ptr noundef %131)
  %133 = getelementptr inbounds %struct.s17, ptr %24, i32 0, i32 0
  %134 = getelementptr inbounds [17 x i8], ptr %133, i64 0, i64 0
  %135 = call i32 (ptr, ...) @printf(ptr noundef @.str.16, ptr noundef %134)
  %136 = call float @fr_hfa11()
  %137 = getelementptr inbounds %struct.hfa11, ptr %25, i32 0, i32 0
  store float %136, ptr %137, align 4
  %138 = getelementptr inbounds %struct.hfa11, ptr %25, i32 0, i32 0
  %139 = load float, ptr %138, align 4
  %140 = fpext float %139 to double
  %141 = call i32 (ptr, ...) @printf(ptr noundef @.str.17, double noundef %140)
  %142 = call <2 x float> @fr_hfa12()
  store <2 x float> %142, ptr %26, align 4
  %143 = getelementptr inbounds %struct.hfa12, ptr %26, i32 0, i32 0
  %144 = load float, ptr %143, align 4
  %145 = fpext float %144 to double
  %146 = call <2 x float> @fr_hfa12()
  store <2 x float> %146, ptr %27, align 4
  %147 = getelementptr inbounds %struct.hfa12, ptr %27, i32 0, i32 1
  %148 = load float, ptr %147, align 4
  %149 = fpext float %148 to double
  %150 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %145, double noundef %149)
  %151 = call { <2 x float>, float } @fr_hfa13()
  store { <2 x float>, float } %151, ptr %29, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %28, ptr align 8 %29, i64 12, i1 false)
  %152 = getelementptr inbounds %struct.hfa13, ptr %28, i32 0, i32 0
  %153 = load float, ptr %152, align 4
  %154 = fpext float %153 to double
  %155 = call { <2 x float>, float } @fr_hfa13()
  store { <2 x float>, float } %155, ptr %31, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %30, ptr align 8 %31, i64 12, i1 false)
  %156 = getelementptr inbounds %struct.hfa13, ptr %30, i32 0, i32 2
  %157 = load float, ptr %156, align 4
  %158 = fpext float %157 to double
  %159 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %154, double noundef %158)
  %160 = call { <2 x float>, <2 x float> } @fr_hfa14()
  %161 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %32, i32 0, i32 0
  %162 = extractvalue { <2 x float>, <2 x float> } %160, 0
  store <2 x float> %162, ptr %161, align 4
  %163 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %32, i32 0, i32 1
  %164 = extractvalue { <2 x float>, <2 x float> } %160, 1
  store <2 x float> %164, ptr %163, align 4
  %165 = getelementptr inbounds %struct.hfa14, ptr %32, i32 0, i32 0
  %166 = load float, ptr %165, align 4
  %167 = fpext float %166 to double
  %168 = call { <2 x float>, <2 x float> } @fr_hfa14()
  %169 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %33, i32 0, i32 0
  %170 = extractvalue { <2 x float>, <2 x float> } %168, 0
  store <2 x float> %170, ptr %169, align 4
  %171 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %33, i32 0, i32 1
  %172 = extractvalue { <2 x float>, <2 x float> } %168, 1
  store <2 x float> %172, ptr %171, align 4
  %173 = getelementptr inbounds %struct.hfa14, ptr %33, i32 0, i32 3
  %174 = load float, ptr %173, align 4
  %175 = fpext float %174 to double
  %176 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %167, double noundef %175)
  %177 = call double @fr_hfa21()
  %178 = getelementptr inbounds %struct.hfa21, ptr %34, i32 0, i32 0
  store double %177, ptr %178, align 8
  %179 = getelementptr inbounds %struct.hfa21, ptr %34, i32 0, i32 0
  %180 = load double, ptr %179, align 8
  %181 = call i32 (ptr, ...) @printf(ptr noundef @.str.17, double noundef %180)
  %182 = call { double, double } @fr_hfa22()
  %183 = getelementptr inbounds { double, double }, ptr %35, i32 0, i32 0
  %184 = extractvalue { double, double } %182, 0
  store double %184, ptr %183, align 8
  %185 = getelementptr inbounds { double, double }, ptr %35, i32 0, i32 1
  %186 = extractvalue { double, double } %182, 1
  store double %186, ptr %185, align 8
  %187 = getelementptr inbounds %struct.hfa22, ptr %35, i32 0, i32 0
  %188 = load double, ptr %187, align 8
  %189 = call { double, double } @fr_hfa22()
  %190 = getelementptr inbounds { double, double }, ptr %36, i32 0, i32 0
  %191 = extractvalue { double, double } %189, 0
  store double %191, ptr %190, align 8
  %192 = getelementptr inbounds { double, double }, ptr %36, i32 0, i32 1
  %193 = extractvalue { double, double } %189, 1
  store double %193, ptr %192, align 8
  %194 = getelementptr inbounds %struct.hfa22, ptr %36, i32 0, i32 1
  %195 = load double, ptr %194, align 8
  %196 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %188, double noundef %195)
  call void @fr_hfa23(ptr dead_on_unwind writable sret(%struct.hfa23) align 8 %37)
  %197 = getelementptr inbounds %struct.hfa23, ptr %37, i32 0, i32 0
  %198 = load double, ptr %197, align 8
  call void @fr_hfa23(ptr dead_on_unwind writable sret(%struct.hfa23) align 8 %38)
  %199 = getelementptr inbounds %struct.hfa23, ptr %38, i32 0, i32 2
  %200 = load double, ptr %199, align 8
  %201 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %198, double noundef %200)
  call void @fr_hfa24(ptr dead_on_unwind writable sret(%struct.hfa24) align 8 %39)
  %202 = getelementptr inbounds %struct.hfa24, ptr %39, i32 0, i32 0
  %203 = load double, ptr %202, align 8
  call void @fr_hfa24(ptr dead_on_unwind writable sret(%struct.hfa24) align 8 %40)
  %204 = getelementptr inbounds %struct.hfa24, ptr %40, i32 0, i32 3
  %205 = load double, ptr %204, align 8
  %206 = call i32 (ptr, ...) @printf(ptr noundef @.str.18, double noundef %203, double noundef %205)
  %207 = call x86_fp80 @fr_hfa31()
  store x86_fp80 %207, ptr %41, align 16
  %208 = getelementptr inbounds %struct.hfa31, ptr %41, i32 0, i32 0
  %209 = load x86_fp80, ptr %208, align 16
  %210 = call i32 (ptr, ...) @printf(ptr noundef @.str.21, x86_fp80 noundef %209)
  call void @fr_hfa32(ptr dead_on_unwind writable sret(%struct.hfa32) align 16 %42)
  %211 = getelementptr inbounds %struct.hfa32, ptr %42, i32 0, i32 0
  %212 = load x86_fp80, ptr %211, align 16
  call void @fr_hfa32(ptr dead_on_unwind writable sret(%struct.hfa32) align 16 %43)
  %213 = getelementptr inbounds %struct.hfa32, ptr %43, i32 0, i32 1
  %214 = load x86_fp80, ptr %213, align 16
  %215 = call i32 (ptr, ...) @printf(ptr noundef @.str.22, x86_fp80 noundef %212, x86_fp80 noundef %214)
  call void @fr_hfa33(ptr dead_on_unwind writable sret(%struct.hfa33) align 16 %44)
  %216 = getelementptr inbounds %struct.hfa33, ptr %44, i32 0, i32 0
  %217 = load x86_fp80, ptr %216, align 16
  call void @fr_hfa33(ptr dead_on_unwind writable sret(%struct.hfa33) align 16 %45)
  %218 = getelementptr inbounds %struct.hfa33, ptr %45, i32 0, i32 2
  %219 = load x86_fp80, ptr %218, align 16
  %220 = call i32 (ptr, ...) @printf(ptr noundef @.str.22, x86_fp80 noundef %217, x86_fp80 noundef %219)
  call void @fr_hfa34(ptr dead_on_unwind writable sret(%struct.hfa34) align 16 %46)
  %221 = getelementptr inbounds %struct.hfa34, ptr %46, i32 0, i32 0
  %222 = load x86_fp80, ptr %221, align 16
  call void @fr_hfa34(ptr dead_on_unwind writable sret(%struct.hfa34) align 16 %47)
  %223 = getelementptr inbounds %struct.hfa34, ptr %47, i32 0, i32 3
  %224 = load x86_fp80, ptr %223, align 16
  %225 = call i32 (ptr, ...) @printf(ptr noundef @.str.22, x86_fp80 noundef %222, x86_fp80 noundef %224)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @match(ptr noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  %5 = alloca ptr, align 8
  %6 = alloca ptr, align 8
  store ptr %0, ptr %4, align 8
  store ptr %1, ptr %5, align 8
  %7 = load ptr, ptr %4, align 8
  %8 = load ptr, ptr %7, align 8
  store ptr %8, ptr %6, align 8
  %9 = load ptr, ptr %4, align 8
  %10 = load ptr, ptr %9, align 8
  store ptr %10, ptr %6, align 8
  br label %11

11:                                               ; preds = %27, %2
  %12 = load ptr, ptr %5, align 8
  %13 = load i8, ptr %12, align 1
  %14 = sext i8 %13 to i32
  %15 = icmp ne i32 %14, 0
  br i1 %15, label %16, label %24

16:                                               ; preds = %11
  %17 = load ptr, ptr %5, align 8
  %18 = load i8, ptr %17, align 1
  %19 = sext i8 %18 to i32
  %20 = load ptr, ptr %6, align 8
  %21 = load i8, ptr %20, align 1
  %22 = sext i8 %21 to i32
  %23 = icmp eq i32 %19, %22
  br label %24

24:                                               ; preds = %16, %11
  %25 = phi i1 [ false, %11 ], [ %23, %16 ]
  br i1 %25, label %26, label %32

26:                                               ; preds = %24
  br label %27

27:                                               ; preds = %26
  %28 = load ptr, ptr %5, align 8
  %29 = getelementptr inbounds i8, ptr %28, i32 1
  store ptr %29, ptr %5, align 8
  %30 = load ptr, ptr %6, align 8
  %31 = getelementptr inbounds i8, ptr %30, i32 1
  store ptr %31, ptr %6, align 8
  br label %11

32:                                               ; preds = %24
  %33 = load ptr, ptr %5, align 8
  %34 = load i8, ptr %33, align 1
  %35 = icmp ne i8 %34, 0
  br i1 %35, label %40, label %36

36:                                               ; preds = %32
  %37 = load ptr, ptr %6, align 8
  %38 = getelementptr inbounds i8, ptr %37, i64 -1
  %39 = load ptr, ptr %4, align 8
  store ptr %38, ptr %39, align 8
  store i32 1, ptr %3, align 4
  br label %41

40:                                               ; preds = %32
  store i32 0, ptr %3, align 4
  br label %41

41:                                               ; preds = %40, %36
  %42 = load i32, ptr %3, align 4
  ret i32 %42
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @myprintf(ptr noundef %0, ...) #0 {
  %2 = alloca ptr, align 8
  %3 = alloca ptr, align 8
  %4 = alloca [1 x %struct.__va_list_tag], align 16
  %5 = alloca %struct.s7, align 1
  %6 = alloca %struct.s9, align 1
  %7 = alloca %struct.hfa11, align 4
  %8 = alloca %struct.hfa12, align 4
  %9 = alloca %struct.hfa13, align 4
  %10 = alloca %struct.hfa13, align 4
  %11 = alloca %struct.hfa14, align 4
  %12 = alloca %struct.hfa14, align 4
  %13 = alloca %struct.hfa21, align 8
  %14 = alloca %struct.hfa22, align 8
  %15 = alloca %struct.hfa22, align 8
  %16 = alloca %struct.hfa23, align 8
  %17 = alloca %struct.hfa24, align 8
  %18 = alloca %struct.hfa31, align 16
  %19 = alloca %struct.hfa32, align 16
  %20 = alloca %struct.hfa33, align 16
  %21 = alloca %struct.hfa34, align 16
  store ptr %0, ptr %2, align 8
  %22 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %22)
  %23 = load ptr, ptr %2, align 8
  store ptr %23, ptr %3, align 8
  br label %24

24:                                               ; preds = %344, %1
  %25 = load ptr, ptr %3, align 8
  %26 = load i8, ptr %25, align 1
  %27 = icmp ne i8 %26, 0
  br i1 %27, label %28, label %347

28:                                               ; preds = %24
  %29 = call i32 @match(ptr noundef %3, ptr noundef @.str.30)
  %30 = icmp ne i32 %29, 0
  br i1 %30, label %31, label %50

31:                                               ; preds = %28
  %32 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %33 = getelementptr inbounds %struct.__va_list_tag, ptr %32, i32 0, i32 0
  %34 = load i32, ptr %33, align 16
  %35 = icmp ule i32 %34, 40
  br i1 %35, label %36, label %41

36:                                               ; preds = %31
  %37 = getelementptr inbounds %struct.__va_list_tag, ptr %32, i32 0, i32 3
  %38 = load ptr, ptr %37, align 16
  %39 = getelementptr i8, ptr %38, i32 %34
  %40 = add i32 %34, 8
  store i32 %40, ptr %33, align 16
  br label %45

41:                                               ; preds = %31
  %42 = getelementptr inbounds %struct.__va_list_tag, ptr %32, i32 0, i32 2
  %43 = load ptr, ptr %42, align 8
  %44 = getelementptr i8, ptr %43, i32 8
  store ptr %44, ptr %42, align 8
  br label %45

45:                                               ; preds = %41, %36
  %46 = phi ptr [ %39, %36 ], [ %43, %41 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %5, ptr align 1 %46, i64 7, i1 false)
  %47 = getelementptr inbounds %struct.s7, ptr %5, i32 0, i32 0
  %48 = getelementptr inbounds [7 x i8], ptr %47, i64 0, i64 0
  %49 = call i32 (ptr, ...) @printf(ptr noundef @.str.31, ptr noundef %48)
  br label %343

50:                                               ; preds = %28
  %51 = call i32 @match(ptr noundef %3, ptr noundef @.str.32)
  %52 = icmp ne i32 %51, 0
  br i1 %52, label %53, label %72

53:                                               ; preds = %50
  %54 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %55 = getelementptr inbounds %struct.__va_list_tag, ptr %54, i32 0, i32 0
  %56 = load i32, ptr %55, align 16
  %57 = icmp ule i32 %56, 32
  br i1 %57, label %58, label %63

58:                                               ; preds = %53
  %59 = getelementptr inbounds %struct.__va_list_tag, ptr %54, i32 0, i32 3
  %60 = load ptr, ptr %59, align 16
  %61 = getelementptr i8, ptr %60, i32 %56
  %62 = add i32 %56, 16
  store i32 %62, ptr %55, align 16
  br label %67

63:                                               ; preds = %53
  %64 = getelementptr inbounds %struct.__va_list_tag, ptr %54, i32 0, i32 2
  %65 = load ptr, ptr %64, align 8
  %66 = getelementptr i8, ptr %65, i32 16
  store ptr %66, ptr %64, align 8
  br label %67

67:                                               ; preds = %63, %58
  %68 = phi ptr [ %61, %58 ], [ %65, %63 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %6, ptr align 1 %68, i64 9, i1 false)
  %69 = getelementptr inbounds %struct.s9, ptr %6, i32 0, i32 0
  %70 = getelementptr inbounds [9 x i8], ptr %69, i64 0, i64 0
  %71 = call i32 (ptr, ...) @printf(ptr noundef @.str.33, ptr noundef %70)
  br label %342

72:                                               ; preds = %50
  %73 = call i32 @match(ptr noundef %3, ptr noundef @.str.34)
  %74 = icmp ne i32 %73, 0
  br i1 %74, label %75, label %98

75:                                               ; preds = %72
  %76 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %77 = getelementptr inbounds %struct.__va_list_tag, ptr %76, i32 0, i32 1
  %78 = load i32, ptr %77, align 4
  %79 = icmp ule i32 %78, 160
  br i1 %79, label %80, label %85

80:                                               ; preds = %75
  %81 = getelementptr inbounds %struct.__va_list_tag, ptr %76, i32 0, i32 3
  %82 = load ptr, ptr %81, align 16
  %83 = getelementptr i8, ptr %82, i32 %78
  %84 = add i32 %78, 16
  store i32 %84, ptr %77, align 4
  br label %89

85:                                               ; preds = %75
  %86 = getelementptr inbounds %struct.__va_list_tag, ptr %76, i32 0, i32 2
  %87 = load ptr, ptr %86, align 8
  %88 = getelementptr i8, ptr %87, i32 8
  store ptr %88, ptr %86, align 8
  br label %89

89:                                               ; preds = %85, %80
  %90 = phi ptr [ %83, %80 ], [ %87, %85 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %7, ptr align 4 %90, i64 4, i1 false)
  %91 = getelementptr inbounds %struct.hfa11, ptr %7, i32 0, i32 0
  %92 = load float, ptr %91, align 4
  %93 = fpext float %92 to double
  %94 = getelementptr inbounds %struct.hfa11, ptr %7, i32 0, i32 0
  %95 = load float, ptr %94, align 4
  %96 = fpext float %95 to double
  %97 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %93, double noundef %96)
  br label %341

98:                                               ; preds = %72
  %99 = call i32 @match(ptr noundef %3, ptr noundef @.str.36)
  %100 = icmp ne i32 %99, 0
  br i1 %100, label %101, label %124

101:                                              ; preds = %98
  %102 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %103 = getelementptr inbounds %struct.__va_list_tag, ptr %102, i32 0, i32 1
  %104 = load i32, ptr %103, align 4
  %105 = icmp ule i32 %104, 160
  br i1 %105, label %106, label %111

106:                                              ; preds = %101
  %107 = getelementptr inbounds %struct.__va_list_tag, ptr %102, i32 0, i32 3
  %108 = load ptr, ptr %107, align 16
  %109 = getelementptr i8, ptr %108, i32 %104
  %110 = add i32 %104, 16
  store i32 %110, ptr %103, align 4
  br label %115

111:                                              ; preds = %101
  %112 = getelementptr inbounds %struct.__va_list_tag, ptr %102, i32 0, i32 2
  %113 = load ptr, ptr %112, align 8
  %114 = getelementptr i8, ptr %113, i32 8
  store ptr %114, ptr %112, align 8
  br label %115

115:                                              ; preds = %111, %106
  %116 = phi ptr [ %109, %106 ], [ %113, %111 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %8, ptr align 4 %116, i64 8, i1 false)
  %117 = getelementptr inbounds %struct.hfa12, ptr %8, i32 0, i32 0
  %118 = load float, ptr %117, align 4
  %119 = fpext float %118 to double
  %120 = getelementptr inbounds %struct.hfa12, ptr %8, i32 0, i32 1
  %121 = load float, ptr %120, align 4
  %122 = fpext float %121 to double
  %123 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %119, double noundef %122)
  br label %340

124:                                              ; preds = %98
  %125 = call i32 @match(ptr noundef %3, ptr noundef @.str.37)
  %126 = icmp ne i32 %125, 0
  br i1 %126, label %127, label %155

127:                                              ; preds = %124
  %128 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %129 = getelementptr inbounds %struct.__va_list_tag, ptr %128, i32 0, i32 1
  %130 = load i32, ptr %129, align 4
  %131 = icmp ule i32 %130, 144
  br i1 %131, label %132, label %142

132:                                              ; preds = %127
  %133 = getelementptr inbounds %struct.__va_list_tag, ptr %128, i32 0, i32 3
  %134 = load ptr, ptr %133, align 16
  %135 = getelementptr i8, ptr %134, i32 %130
  %136 = getelementptr inbounds i8, ptr %135, i64 16
  %137 = load <2 x float>, ptr %135, align 16
  %138 = getelementptr inbounds { <2 x float>, float }, ptr %10, i32 0, i32 0
  store <2 x float> %137, ptr %138, align 4
  %139 = load float, ptr %136, align 16
  %140 = getelementptr inbounds { <2 x float>, float }, ptr %10, i32 0, i32 1
  store float %139, ptr %140, align 4
  %141 = add i32 %130, 32
  store i32 %141, ptr %129, align 4
  br label %146

142:                                              ; preds = %127
  %143 = getelementptr inbounds %struct.__va_list_tag, ptr %128, i32 0, i32 2
  %144 = load ptr, ptr %143, align 8
  %145 = getelementptr i8, ptr %144, i32 16
  store ptr %145, ptr %143, align 8
  br label %146

146:                                              ; preds = %142, %132
  %147 = phi ptr [ %10, %132 ], [ %144, %142 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %9, ptr align 4 %147, i64 12, i1 false)
  %148 = getelementptr inbounds %struct.hfa13, ptr %9, i32 0, i32 0
  %149 = load float, ptr %148, align 4
  %150 = fpext float %149 to double
  %151 = getelementptr inbounds %struct.hfa13, ptr %9, i32 0, i32 2
  %152 = load float, ptr %151, align 4
  %153 = fpext float %152 to double
  %154 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %150, double noundef %153)
  br label %339

155:                                              ; preds = %124
  %156 = call i32 @match(ptr noundef %3, ptr noundef @.str.38)
  %157 = icmp ne i32 %156, 0
  br i1 %157, label %158, label %186

158:                                              ; preds = %155
  %159 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %160 = getelementptr inbounds %struct.__va_list_tag, ptr %159, i32 0, i32 1
  %161 = load i32, ptr %160, align 4
  %162 = icmp ule i32 %161, 144
  br i1 %162, label %163, label %173

163:                                              ; preds = %158
  %164 = getelementptr inbounds %struct.__va_list_tag, ptr %159, i32 0, i32 3
  %165 = load ptr, ptr %164, align 16
  %166 = getelementptr i8, ptr %165, i32 %161
  %167 = getelementptr inbounds i8, ptr %166, i64 16
  %168 = load <2 x float>, ptr %166, align 16
  %169 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %12, i32 0, i32 0
  store <2 x float> %168, ptr %169, align 4
  %170 = load <2 x float>, ptr %167, align 16
  %171 = getelementptr inbounds { <2 x float>, <2 x float> }, ptr %12, i32 0, i32 1
  store <2 x float> %170, ptr %171, align 4
  %172 = add i32 %161, 32
  store i32 %172, ptr %160, align 4
  br label %177

173:                                              ; preds = %158
  %174 = getelementptr inbounds %struct.__va_list_tag, ptr %159, i32 0, i32 2
  %175 = load ptr, ptr %174, align 8
  %176 = getelementptr i8, ptr %175, i32 16
  store ptr %176, ptr %174, align 8
  br label %177

177:                                              ; preds = %173, %163
  %178 = phi ptr [ %12, %163 ], [ %175, %173 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %11, ptr align 4 %178, i64 16, i1 false)
  %179 = getelementptr inbounds %struct.hfa14, ptr %11, i32 0, i32 0
  %180 = load float, ptr %179, align 4
  %181 = fpext float %180 to double
  %182 = getelementptr inbounds %struct.hfa14, ptr %11, i32 0, i32 3
  %183 = load float, ptr %182, align 4
  %184 = fpext float %183 to double
  %185 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %181, double noundef %184)
  br label %338

186:                                              ; preds = %155
  %187 = call i32 @match(ptr noundef %3, ptr noundef @.str.39)
  %188 = icmp ne i32 %187, 0
  br i1 %188, label %189, label %210

189:                                              ; preds = %186
  %190 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %191 = getelementptr inbounds %struct.__va_list_tag, ptr %190, i32 0, i32 1
  %192 = load i32, ptr %191, align 4
  %193 = icmp ule i32 %192, 160
  br i1 %193, label %194, label %199

194:                                              ; preds = %189
  %195 = getelementptr inbounds %struct.__va_list_tag, ptr %190, i32 0, i32 3
  %196 = load ptr, ptr %195, align 16
  %197 = getelementptr i8, ptr %196, i32 %192
  %198 = add i32 %192, 16
  store i32 %198, ptr %191, align 4
  br label %203

199:                                              ; preds = %189
  %200 = getelementptr inbounds %struct.__va_list_tag, ptr %190, i32 0, i32 2
  %201 = load ptr, ptr %200, align 8
  %202 = getelementptr i8, ptr %201, i32 8
  store ptr %202, ptr %200, align 8
  br label %203

203:                                              ; preds = %199, %194
  %204 = phi ptr [ %197, %194 ], [ %201, %199 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %13, ptr align 8 %204, i64 8, i1 false)
  %205 = getelementptr inbounds %struct.hfa21, ptr %13, i32 0, i32 0
  %206 = load double, ptr %205, align 8
  %207 = getelementptr inbounds %struct.hfa21, ptr %13, i32 0, i32 0
  %208 = load double, ptr %207, align 8
  %209 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %206, double noundef %208)
  br label %337

210:                                              ; preds = %186
  %211 = call i32 @match(ptr noundef %3, ptr noundef @.str.40)
  %212 = icmp ne i32 %211, 0
  br i1 %212, label %213, label %239

213:                                              ; preds = %210
  %214 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %215 = getelementptr inbounds %struct.__va_list_tag, ptr %214, i32 0, i32 1
  %216 = load i32, ptr %215, align 4
  %217 = icmp ule i32 %216, 144
  br i1 %217, label %218, label %228

218:                                              ; preds = %213
  %219 = getelementptr inbounds %struct.__va_list_tag, ptr %214, i32 0, i32 3
  %220 = load ptr, ptr %219, align 16
  %221 = getelementptr i8, ptr %220, i32 %216
  %222 = getelementptr inbounds i8, ptr %221, i64 16
  %223 = load double, ptr %221, align 16
  %224 = getelementptr inbounds { double, double }, ptr %15, i32 0, i32 0
  store double %223, ptr %224, align 8
  %225 = load double, ptr %222, align 16
  %226 = getelementptr inbounds { double, double }, ptr %15, i32 0, i32 1
  store double %225, ptr %226, align 8
  %227 = add i32 %216, 32
  store i32 %227, ptr %215, align 4
  br label %232

228:                                              ; preds = %213
  %229 = getelementptr inbounds %struct.__va_list_tag, ptr %214, i32 0, i32 2
  %230 = load ptr, ptr %229, align 8
  %231 = getelementptr i8, ptr %230, i32 16
  store ptr %231, ptr %229, align 8
  br label %232

232:                                              ; preds = %228, %218
  %233 = phi ptr [ %15, %218 ], [ %230, %228 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %14, ptr align 8 %233, i64 16, i1 false)
  %234 = getelementptr inbounds %struct.hfa22, ptr %14, i32 0, i32 0
  %235 = load double, ptr %234, align 8
  %236 = getelementptr inbounds %struct.hfa22, ptr %14, i32 0, i32 1
  %237 = load double, ptr %236, align 8
  %238 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %235, double noundef %237)
  br label %336

239:                                              ; preds = %210
  %240 = call i32 @match(ptr noundef %3, ptr noundef @.str.41)
  %241 = icmp ne i32 %240, 0
  br i1 %241, label %242, label %252

242:                                              ; preds = %239
  %243 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %244 = getelementptr inbounds %struct.__va_list_tag, ptr %243, i32 0, i32 2
  %245 = load ptr, ptr %244, align 8
  %246 = getelementptr i8, ptr %245, i32 24
  store ptr %246, ptr %244, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %16, ptr align 8 %245, i64 24, i1 false)
  %247 = getelementptr inbounds %struct.hfa23, ptr %16, i32 0, i32 0
  %248 = load double, ptr %247, align 8
  %249 = getelementptr inbounds %struct.hfa23, ptr %16, i32 0, i32 2
  %250 = load double, ptr %249, align 8
  %251 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %248, double noundef %250)
  br label %335

252:                                              ; preds = %239
  %253 = call i32 @match(ptr noundef %3, ptr noundef @.str.42)
  %254 = icmp ne i32 %253, 0
  br i1 %254, label %255, label %265

255:                                              ; preds = %252
  %256 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %257 = getelementptr inbounds %struct.__va_list_tag, ptr %256, i32 0, i32 2
  %258 = load ptr, ptr %257, align 8
  %259 = getelementptr i8, ptr %258, i32 32
  store ptr %259, ptr %257, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %17, ptr align 8 %258, i64 32, i1 false)
  %260 = getelementptr inbounds %struct.hfa24, ptr %17, i32 0, i32 0
  %261 = load double, ptr %260, align 8
  %262 = getelementptr inbounds %struct.hfa24, ptr %17, i32 0, i32 3
  %263 = load double, ptr %262, align 8
  %264 = call i32 (ptr, ...) @printf(ptr noundef @.str.35, double noundef %261, double noundef %263)
  br label %334

265:                                              ; preds = %252
  %266 = call i32 @match(ptr noundef %3, ptr noundef @.str.43)
  %267 = icmp ne i32 %266, 0
  br i1 %267, label %268, label %280

268:                                              ; preds = %265
  %269 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %270 = getelementptr inbounds %struct.__va_list_tag, ptr %269, i32 0, i32 2
  %271 = load ptr, ptr %270, align 8
  %272 = getelementptr inbounds i8, ptr %271, i32 15
  %273 = call ptr @llvm.ptrmask.p0.i64(ptr %272, i64 -16)
  %274 = getelementptr i8, ptr %273, i32 16
  store ptr %274, ptr %270, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %18, ptr align 16 %273, i64 16, i1 false)
  %275 = getelementptr inbounds %struct.hfa31, ptr %18, i32 0, i32 0
  %276 = load x86_fp80, ptr %275, align 16
  %277 = getelementptr inbounds %struct.hfa31, ptr %18, i32 0, i32 0
  %278 = load x86_fp80, ptr %277, align 16
  %279 = call i32 (ptr, ...) @printf(ptr noundef @.str.44, x86_fp80 noundef %276, x86_fp80 noundef %278)
  br label %333

280:                                              ; preds = %265
  %281 = call i32 @match(ptr noundef %3, ptr noundef @.str.45)
  %282 = icmp ne i32 %281, 0
  br i1 %282, label %283, label %295

283:                                              ; preds = %280
  %284 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %285 = getelementptr inbounds %struct.__va_list_tag, ptr %284, i32 0, i32 2
  %286 = load ptr, ptr %285, align 8
  %287 = getelementptr inbounds i8, ptr %286, i32 15
  %288 = call ptr @llvm.ptrmask.p0.i64(ptr %287, i64 -16)
  %289 = getelementptr i8, ptr %288, i32 32
  store ptr %289, ptr %285, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %19, ptr align 16 %288, i64 32, i1 false)
  %290 = getelementptr inbounds %struct.hfa32, ptr %19, i32 0, i32 0
  %291 = load x86_fp80, ptr %290, align 16
  %292 = getelementptr inbounds %struct.hfa32, ptr %19, i32 0, i32 1
  %293 = load x86_fp80, ptr %292, align 16
  %294 = call i32 (ptr, ...) @printf(ptr noundef @.str.44, x86_fp80 noundef %291, x86_fp80 noundef %293)
  br label %332

295:                                              ; preds = %280
  %296 = call i32 @match(ptr noundef %3, ptr noundef @.str.46)
  %297 = icmp ne i32 %296, 0
  br i1 %297, label %298, label %310

298:                                              ; preds = %295
  %299 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %300 = getelementptr inbounds %struct.__va_list_tag, ptr %299, i32 0, i32 2
  %301 = load ptr, ptr %300, align 8
  %302 = getelementptr inbounds i8, ptr %301, i32 15
  %303 = call ptr @llvm.ptrmask.p0.i64(ptr %302, i64 -16)
  %304 = getelementptr i8, ptr %303, i32 48
  store ptr %304, ptr %300, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %20, ptr align 16 %303, i64 48, i1 false)
  %305 = getelementptr inbounds %struct.hfa33, ptr %20, i32 0, i32 0
  %306 = load x86_fp80, ptr %305, align 16
  %307 = getelementptr inbounds %struct.hfa33, ptr %20, i32 0, i32 2
  %308 = load x86_fp80, ptr %307, align 16
  %309 = call i32 (ptr, ...) @printf(ptr noundef @.str.44, x86_fp80 noundef %306, x86_fp80 noundef %308)
  br label %331

310:                                              ; preds = %295
  %311 = call i32 @match(ptr noundef %3, ptr noundef @.str.47)
  %312 = icmp ne i32 %311, 0
  br i1 %312, label %313, label %325

313:                                              ; preds = %310
  %314 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %4, i64 0, i64 0
  %315 = getelementptr inbounds %struct.__va_list_tag, ptr %314, i32 0, i32 2
  %316 = load ptr, ptr %315, align 8
  %317 = getelementptr inbounds i8, ptr %316, i32 15
  %318 = call ptr @llvm.ptrmask.p0.i64(ptr %317, i64 -16)
  %319 = getelementptr i8, ptr %318, i32 64
  store ptr %319, ptr %315, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 16 %21, ptr align 16 %318, i64 64, i1 false)
  %320 = getelementptr inbounds %struct.hfa34, ptr %21, i32 0, i32 0
  %321 = load x86_fp80, ptr %320, align 16
  %322 = getelementptr inbounds %struct.hfa34, ptr %21, i32 0, i32 3
  %323 = load x86_fp80, ptr %322, align 16
  %324 = call i32 (ptr, ...) @printf(ptr noundef @.str.44, x86_fp80 noundef %321, x86_fp80 noundef %323)
  br label %330

325:                                              ; preds = %310
  %326 = load ptr, ptr %3, align 8
  %327 = load i8, ptr %326, align 1
  %328 = sext i8 %327 to i32
  %329 = call i32 @putchar(i32 noundef %328)
  br label %330

330:                                              ; preds = %325, %313
  br label %331

331:                                              ; preds = %330, %298
  br label %332

332:                                              ; preds = %331, %283
  br label %333

333:                                              ; preds = %332, %268
  br label %334

334:                                              ; preds = %333, %255
  br label %335

335:                                              ; preds = %334, %242
  br label %336

336:                                              ; preds = %335, %232
  br label %337

337:                                              ; preds = %336, %203
  br label %338

338:                                              ; preds = %337, %177
  br label %339

339:                                              ; preds = %338, %146
  br label %340

340:                                              ; preds = %339, %115
  br label %341

341:                                              ; preds = %340, %89
  br label %342

342:                                              ; preds = %341, %67
  br label %343

343:                                              ; preds = %342, %45
  br label %344

344:                                              ; preds = %343
  %345 = load ptr, ptr %3, align 8
  %346 = getelementptr inbounds i8, ptr %345, i32 1
  store ptr %346, ptr %3, align 8
  br label %24

347:                                              ; preds = %24
  %348 = call i32 @putchar(i32 noundef 10)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #4

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare ptr @llvm.ptrmask.p0.i64(ptr, i64) #5

declare i32 @putchar(i32 noundef) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @stdarg() #3 {
  %1 = alloca { i64, i8 }, align 1
  %2 = alloca { i64, i8 }, align 1
  %3 = alloca i56, align 8
  %4 = alloca { i64, i8 }, align 1
  %5 = alloca { i64, i8 }, align 1
  %6 = alloca { <2 x float>, float }, align 4
  %7 = alloca { <2 x float>, float }, align 4
  %8 = alloca { <2 x float>, float }, align 4
  %9 = alloca { <2 x float>, float }, align 4
  %10 = alloca { <2 x float>, float }, align 4
  %11 = alloca { <2 x float>, float }, align 4
  %12 = alloca { <2 x float>, float }, align 4
  %13 = alloca { <2 x float>, float }, align 4
  %14 = alloca { <2 x float>, float }, align 4
  %15 = alloca { <2 x float>, float }, align 4
  %16 = alloca { <2 x float>, float }, align 4
  %17 = alloca { <2 x float>, float }, align 4
  %18 = call i32 (ptr, ...) @printf(ptr noundef @.str.48)
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %1, ptr align 1 @s9, i64 9, i1 false)
  %19 = getelementptr inbounds { i64, i8 }, ptr %1, i32 0, i32 0
  %20 = load i64, ptr %19, align 1
  %21 = getelementptr inbounds { i64, i8 }, ptr %1, i32 0, i32 1
  %22 = load i8, ptr %21, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %2, ptr align 1 @s9, i64 9, i1 false)
  %23 = getelementptr inbounds { i64, i8 }, ptr %2, i32 0, i32 0
  %24 = load i64, ptr %23, align 1
  %25 = getelementptr inbounds { i64, i8 }, ptr %2, i32 0, i32 1
  %26 = load i8, ptr %25, align 1
  call void (ptr, ...) @myprintf(ptr noundef @.str.49, i64 %20, i8 %22, i64 %24, i8 %26, ptr noundef byval(%struct.s9) align 8 @s9, ptr noundef byval(%struct.s9) align 8 @s9, ptr noundef byval(%struct.s9) align 8 @s9, ptr noundef byval(%struct.s9) align 8 @s9)
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %3, ptr align 1 @s7, i64 7, i1 false)
  %27 = load i56, ptr %3, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %4, ptr align 1 @s9, i64 9, i1 false)
  %28 = getelementptr inbounds { i64, i8 }, ptr %4, i32 0, i32 0
  %29 = load i64, ptr %28, align 1
  %30 = getelementptr inbounds { i64, i8 }, ptr %4, i32 0, i32 1
  %31 = load i8, ptr %30, align 1
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %5, ptr align 1 @s9, i64 9, i1 false)
  %32 = getelementptr inbounds { i64, i8 }, ptr %5, i32 0, i32 0
  %33 = load i64, ptr %32, align 1
  %34 = getelementptr inbounds { i64, i8 }, ptr %5, i32 0, i32 1
  %35 = load i8, ptr %34, align 1
  call void (ptr, ...) @myprintf(ptr noundef @.str.50, i56 %27, i64 %29, i8 %31, i64 %33, i8 %35, ptr noundef byval(%struct.s9) align 8 @s9, ptr noundef byval(%struct.s9) align 8 @s9, ptr noundef byval(%struct.s9) align 8 @s9)
  call void (ptr, ...) @myprintf(ptr noundef @.str.51)
  call void (ptr, ...) @myprintf(ptr noundef @.str.52, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34)
  call void (ptr, ...) @myprintf(ptr noundef @.str.53, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34)
  call void (ptr, ...) @myprintf(ptr noundef @.str.54, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34)
  call void (ptr, ...) @myprintf(ptr noundef @.str.55, ptr noundef byval(%struct.hfa31) align 16 @hfa31, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa34) align 16 @hfa34)
  call void (ptr, ...) @myprintf(ptr noundef @.str.56, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33)
  call void (ptr, ...) @myprintf(ptr noundef @.str.57, ptr noundef byval(%struct.hfa31) align 16 @hfa31, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33)
  call void (ptr, ...) @myprintf(ptr noundef @.str.58, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa33) align 16 @hfa33)
  call void (ptr, ...) @myprintf(ptr noundef @.str.59, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa32) align 16 @hfa32)
  call void (ptr, ...) @myprintf(ptr noundef @.str.60, ptr noundef byval(%struct.hfa33) align 16 @hfa33, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa32) align 16 @hfa32)
  call void (ptr, ...) @myprintf(ptr noundef @.str.61, ptr noundef byval(%struct.hfa34) align 16 @hfa34, ptr noundef byval(%struct.hfa32) align 16 @hfa32, ptr noundef byval(%struct.hfa31) align 16 @hfa31, ptr noundef byval(%struct.hfa31) align 16 @hfa31, ptr noundef byval(%struct.hfa31) align 16 @hfa31, ptr noundef byval(%struct.hfa31) align 16 @hfa31)
  call void (ptr, ...) @myprintf(ptr noundef @.str.62)
  call void (ptr, ...) @myprintf(ptr noundef @.str.63, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24)
  call void (ptr, ...) @myprintf(ptr noundef @.str.64, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24)
  %36 = load double, ptr @hfa22, align 8
  %37 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.65, double %36, double %37, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24)
  %38 = load double, ptr @hfa21, align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.66, double %38, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24, ptr noundef byval(%struct.hfa24) align 8 @hfa24)
  %39 = load double, ptr @hfa22, align 8
  %40 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.67, double %39, double %40, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23)
  %41 = load double, ptr @hfa21, align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.68, double %41, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23)
  call void (ptr, ...) @myprintf(ptr noundef @.str.69, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23, ptr noundef byval(%struct.hfa23) align 8 @hfa23)
  %42 = load double, ptr @hfa22, align 8
  %43 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %44 = load double, ptr @hfa22, align 8
  %45 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %46 = load double, ptr @hfa22, align 8
  %47 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %48 = load double, ptr @hfa22, align 8
  %49 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.70, ptr noundef byval(%struct.hfa24) align 8 @hfa24, double %42, double %43, double %44, double %45, double %46, double %47, double %48, double %49)
  %50 = load double, ptr @hfa22, align 8
  %51 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %52 = load double, ptr @hfa22, align 8
  %53 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %54 = load double, ptr @hfa22, align 8
  %55 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %56 = load double, ptr @hfa22, align 8
  %57 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.71, ptr noundef byval(%struct.hfa23) align 8 @hfa23, double %50, double %51, double %52, double %53, double %54, double %55, double %56, double %57)
  %58 = load double, ptr @hfa22, align 8
  %59 = load double, ptr getelementptr inbounds ({ double, double }, ptr @hfa22, i32 0, i32 1), align 8
  %60 = load double, ptr @hfa21, align 8
  %61 = load double, ptr @hfa21, align 8
  %62 = load double, ptr @hfa21, align 8
  %63 = load double, ptr @hfa21, align 8
  call void (ptr, ...) @myprintf(ptr noundef @.str.72, ptr noundef byval(%struct.hfa24) align 8 @hfa24, double %58, double %59, double %60, double %61, double %62, double %63)
  call void (ptr, ...) @myprintf(ptr noundef @.str.73)
  %64 = load <2 x float>, ptr @hfa14, align 4
  %65 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %66 = load <2 x float>, ptr @hfa14, align 4
  %67 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %68 = load <2 x float>, ptr @hfa14, align 4
  %69 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %70 = load <2 x float>, ptr @hfa14, align 4
  %71 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.74, <2 x float> %64, <2 x float> %65, <2 x float> %66, <2 x float> %67, <2 x float> %68, <2 x float> %69, <2 x float> %70, <2 x float> %71)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %6, ptr align 4 @hfa13, i64 12, i1 false)
  %72 = getelementptr inbounds { <2 x float>, float }, ptr %6, i32 0, i32 0
  %73 = load <2 x float>, ptr %72, align 4
  %74 = getelementptr inbounds { <2 x float>, float }, ptr %6, i32 0, i32 1
  %75 = load float, ptr %74, align 4
  %76 = load <2 x float>, ptr @hfa14, align 4
  %77 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %78 = load <2 x float>, ptr @hfa14, align 4
  %79 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %80 = load <2 x float>, ptr @hfa14, align 4
  %81 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.75, <2 x float> %73, float %75, <2 x float> %76, <2 x float> %77, <2 x float> %78, <2 x float> %79, <2 x float> %80, <2 x float> %81)
  %82 = load <2 x float>, ptr @hfa12, align 4
  %83 = load <2 x float>, ptr @hfa14, align 4
  %84 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %85 = load <2 x float>, ptr @hfa14, align 4
  %86 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %87 = load <2 x float>, ptr @hfa14, align 4
  %88 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.76, <2 x float> %82, <2 x float> %83, <2 x float> %84, <2 x float> %85, <2 x float> %86, <2 x float> %87, <2 x float> %88)
  %89 = load float, ptr @hfa11, align 4
  %90 = load <2 x float>, ptr @hfa14, align 4
  %91 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %92 = load <2 x float>, ptr @hfa14, align 4
  %93 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %94 = load <2 x float>, ptr @hfa14, align 4
  %95 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.77, float %89, <2 x float> %90, <2 x float> %91, <2 x float> %92, <2 x float> %93, <2 x float> %94, <2 x float> %95)
  %96 = load <2 x float>, ptr @hfa12, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %7, ptr align 4 @hfa13, i64 12, i1 false)
  %97 = getelementptr inbounds { <2 x float>, float }, ptr %7, i32 0, i32 0
  %98 = load <2 x float>, ptr %97, align 4
  %99 = getelementptr inbounds { <2 x float>, float }, ptr %7, i32 0, i32 1
  %100 = load float, ptr %99, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %8, ptr align 4 @hfa13, i64 12, i1 false)
  %101 = getelementptr inbounds { <2 x float>, float }, ptr %8, i32 0, i32 0
  %102 = load <2 x float>, ptr %101, align 4
  %103 = getelementptr inbounds { <2 x float>, float }, ptr %8, i32 0, i32 1
  %104 = load float, ptr %103, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %9, ptr align 4 @hfa13, i64 12, i1 false)
  %105 = getelementptr inbounds { <2 x float>, float }, ptr %9, i32 0, i32 0
  %106 = load <2 x float>, ptr %105, align 4
  %107 = getelementptr inbounds { <2 x float>, float }, ptr %9, i32 0, i32 1
  %108 = load float, ptr %107, align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.78, <2 x float> %96, <2 x float> %98, float %100, <2 x float> %102, float %104, <2 x float> %106, float %108, ptr noundef byval(%struct.hfa13) align 8 @hfa13)
  %109 = load float, ptr @hfa11, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %10, ptr align 4 @hfa13, i64 12, i1 false)
  %110 = getelementptr inbounds { <2 x float>, float }, ptr %10, i32 0, i32 0
  %111 = load <2 x float>, ptr %110, align 4
  %112 = getelementptr inbounds { <2 x float>, float }, ptr %10, i32 0, i32 1
  %113 = load float, ptr %112, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %11, ptr align 4 @hfa13, i64 12, i1 false)
  %114 = getelementptr inbounds { <2 x float>, float }, ptr %11, i32 0, i32 0
  %115 = load <2 x float>, ptr %114, align 4
  %116 = getelementptr inbounds { <2 x float>, float }, ptr %11, i32 0, i32 1
  %117 = load float, ptr %116, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %12, ptr align 4 @hfa13, i64 12, i1 false)
  %118 = getelementptr inbounds { <2 x float>, float }, ptr %12, i32 0, i32 0
  %119 = load <2 x float>, ptr %118, align 4
  %120 = getelementptr inbounds { <2 x float>, float }, ptr %12, i32 0, i32 1
  %121 = load float, ptr %120, align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.79, float %109, <2 x float> %111, float %113, <2 x float> %115, float %117, <2 x float> %119, float %121, ptr noundef byval(%struct.hfa13) align 8 @hfa13)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %13, ptr align 4 @hfa13, i64 12, i1 false)
  %122 = getelementptr inbounds { <2 x float>, float }, ptr %13, i32 0, i32 0
  %123 = load <2 x float>, ptr %122, align 4
  %124 = getelementptr inbounds { <2 x float>, float }, ptr %13, i32 0, i32 1
  %125 = load float, ptr %124, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %14, ptr align 4 @hfa13, i64 12, i1 false)
  %126 = getelementptr inbounds { <2 x float>, float }, ptr %14, i32 0, i32 0
  %127 = load <2 x float>, ptr %126, align 4
  %128 = getelementptr inbounds { <2 x float>, float }, ptr %14, i32 0, i32 1
  %129 = load float, ptr %128, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %15, ptr align 4 @hfa13, i64 12, i1 false)
  %130 = getelementptr inbounds { <2 x float>, float }, ptr %15, i32 0, i32 0
  %131 = load <2 x float>, ptr %130, align 4
  %132 = getelementptr inbounds { <2 x float>, float }, ptr %15, i32 0, i32 1
  %133 = load float, ptr %132, align 4
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %16, ptr align 4 @hfa13, i64 12, i1 false)
  %134 = getelementptr inbounds { <2 x float>, float }, ptr %16, i32 0, i32 0
  %135 = load <2 x float>, ptr %134, align 4
  %136 = getelementptr inbounds { <2 x float>, float }, ptr %16, i32 0, i32 1
  %137 = load float, ptr %136, align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.80, <2 x float> %123, float %125, <2 x float> %127, float %129, <2 x float> %131, float %133, <2 x float> %135, float %137)
  %138 = load <2 x float>, ptr @hfa14, align 4
  %139 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %140 = load <2 x float>, ptr @hfa12, align 4
  %141 = load <2 x float>, ptr @hfa12, align 4
  %142 = load <2 x float>, ptr @hfa12, align 4
  %143 = load <2 x float>, ptr @hfa12, align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.81, <2 x float> %138, <2 x float> %139, <2 x float> %140, <2 x float> %141, <2 x float> %142, <2 x float> %143)
  call void @llvm.memcpy.p0.p0.i64(ptr align 4 %17, ptr align 4 @hfa13, i64 12, i1 false)
  %144 = getelementptr inbounds { <2 x float>, float }, ptr %17, i32 0, i32 0
  %145 = load <2 x float>, ptr %144, align 4
  %146 = getelementptr inbounds { <2 x float>, float }, ptr %17, i32 0, i32 1
  %147 = load float, ptr %146, align 4
  %148 = load <2 x float>, ptr @hfa12, align 4
  %149 = load <2 x float>, ptr @hfa12, align 4
  %150 = load <2 x float>, ptr @hfa12, align 4
  %151 = load <2 x float>, ptr @hfa12, align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.82, <2 x float> %145, float %147, <2 x float> %148, <2 x float> %149, <2 x float> %150, <2 x float> %151)
  %152 = load <2 x float>, ptr @hfa14, align 4
  %153 = load <2 x float>, ptr getelementptr inbounds ({ <2 x float>, <2 x float> }, ptr @hfa14, i32 0, i32 1), align 4
  %154 = load <2 x float>, ptr @hfa12, align 4
  %155 = load float, ptr @hfa11, align 4
  %156 = load float, ptr @hfa11, align 4
  %157 = load float, ptr @hfa11, align 4
  %158 = load float, ptr @hfa11, align 4
  call void (ptr, ...) @myprintf(ptr noundef @.str.83, <2 x float> %152, <2 x float> %153, <2 x float> %154, float %155, float %156, float %157, float %158)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @pll(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = call i32 (ptr, ...) @printf(ptr noundef @.str.84, i64 noundef %3)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @movi() #0 {
  %1 = call i32 (ptr, ...) @printf(ptr noundef @.str.85)
  call void @pll(i64 noundef 0)
  call void @pll(i64 noundef 43981)
  call void @pll(i64 noundef 2882338816)
  call void @pll(i64 noundef 188896956645376)
  call void @pll(i64 noundef -6067193122998190080)
  call void @pll(i64 noundef 4294945741)
  call void @pll(i64 noundef 2882404351)
  call void @pll(i64 noundef -21555)
  call void @pll(i64 noundef -1412562945)
  call void @pll(i64 noundef -92573725097985)
  call void @pll(i64 noundef -6066911648021479425)
  call void @pll(i64 noundef 2863311530)
  call void @pll(i64 noundef 6148914691236517205)
  call void @pll(i64 noundef 2004318071)
  call void @pll(i64 noundef 3689348814741910323)
  call void @pll(i64 noundef 4177066232)
  call void @pll(i64 noundef 2170205185142300190)
  call void @pll(i64 noundef 1065369472)
  call void @pll(i64 noundef 143835907860922879)
  call void @pll(i64 noundef 8388544)
  call void @pll(i64 noundef 288221580125796352)
  call void @pll(i64 noundef 2251799813684736)
  call void @pll(i64 noundef 2882343476)
  call void @pll(i64 noundef 188896956650036)
  call void @pll(i64 noundef -6067193122998185420)
  call void @pll(i64 noundef 188897262043136)
  call void @pll(i64 noundef -6067193122692792320)
  call void @pll(i64 noundef -6067173108450590720)
  call void @pll(i64 noundef -1412623820)
  call void @pll(i64 noundef -92573725158860)
  call void @pll(i64 noundef -6066911648021540300)
  call void @pll(i64 noundef -92577714601985)
  call void @pll(i64 noundef -6066911652010983425)
  call void @pll(i64 noundef -6067173104155623425)
  call void @pll(i64 noundef -18686810953847)
  call void @pll(i64 noundef -6066930334832394241)
  call void @pll(i64 noundef -6066930334832433271)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @opi() #0 {
  %1 = alloca i32, align 4
  store i32 1000, ptr %1, align 4
  %2 = load i32, ptr %1, align 4
  %3 = call i32 @addip0(i32 noundef %2)
  %4 = zext i32 %3 to i64
  call void @pll(i64 noundef %4)
  %5 = load i32, ptr %1, align 4
  %6 = sext i32 %5 to i64
  %7 = call i64 @sublp0(i64 noundef %6)
  call void @pll(i64 noundef %7)
  %8 = load i32, ptr %1, align 4
  %9 = call i32 @addip123(i32 noundef %8)
  %10 = zext i32 %9 to i64
  call void @pll(i64 noundef %10)
  %11 = load i32, ptr %1, align 4
  %12 = sext i32 %11 to i64
  %13 = call i64 @addlm123(i64 noundef %12)
  call void @pll(i64 noundef %13)
  %14 = load i32, ptr %1, align 4
  %15 = sext i32 %14 to i64
  %16 = call i64 @sublp4095(i64 noundef %15)
  call void @pll(i64 noundef %16)
  %17 = load i32, ptr %1, align 4
  %18 = call i32 @subim503808(i32 noundef %17)
  %19 = zext i32 %18 to i64
  call void @pll(i64 noundef %19)
  %20 = load i32, ptr %1, align 4
  %21 = sext i32 %20 to i64
  %22 = call i64 @addp12345(i64 noundef %21)
  call void @pll(i64 noundef %22)
  %23 = load i32, ptr %1, align 4
  %24 = call i32 @subp12345(i32 noundef %23)
  %25 = zext i32 %24 to i64
  call void @pll(i64 noundef %25)
  %26 = load i32, ptr %1, align 4
  %27 = call i32 @mvni(i32 noundef %26)
  %28 = zext i32 %27 to i64
  call void @pll(i64 noundef %28)
  %29 = load i32, ptr %1, align 4
  %30 = sext i32 %29 to i64
  %31 = call i64 @negl(i64 noundef %30)
  call void @pll(i64 noundef %31)
  %32 = load i32, ptr %1, align 4
  %33 = call i32 @rsbi123(i32 noundef %32)
  %34 = zext i32 %33 to i64
  call void @pll(i64 noundef %34)
  %35 = load i32, ptr %1, align 4
  %36 = sext i32 %35 to i64
  %37 = call i64 @rsbl123(i64 noundef %36)
  call void @pll(i64 noundef %37)
  %38 = load i32, ptr %1, align 4
  %39 = call i32 @andi0(i32 noundef %38)
  %40 = zext i32 %39 to i64
  call void @pll(i64 noundef %40)
  %41 = load i32, ptr %1, align 4
  %42 = sext i32 %41 to i64
  %43 = call i64 @andlm1(i64 noundef %42)
  call void @pll(i64 noundef %43)
  %44 = load i32, ptr %1, align 4
  %45 = sext i32 %44 to i64
  %46 = call i64 @orrl0(i64 noundef %45)
  call void @pll(i64 noundef %46)
  %47 = load i32, ptr %1, align 4
  %48 = call i32 @orrim1(i32 noundef %47)
  %49 = zext i32 %48 to i64
  call void @pll(i64 noundef %49)
  %50 = load i32, ptr %1, align 4
  %51 = call i32 @eori0(i32 noundef %50)
  %52 = zext i32 %51 to i64
  call void @pll(i64 noundef %52)
  %53 = load i32, ptr %1, align 4
  %54 = sext i32 %53 to i64
  %55 = call i64 @eorlm1(i64 noundef %54)
  call void @pll(i64 noundef %55)
  %56 = load i32, ptr %1, align 4
  %57 = call i32 @and0xf0(i32 noundef %56)
  %58 = zext i32 %57 to i64
  call void @pll(i64 noundef %58)
  %59 = load i32, ptr %1, align 4
  %60 = sext i32 %59 to i64
  %61 = call i64 @orr0xf0(i64 noundef %60)
  call void @pll(i64 noundef %61)
  %62 = load i32, ptr %1, align 4
  %63 = sext i32 %62 to i64
  %64 = call i64 @eor0xf0(i64 noundef %63)
  call void @pll(i64 noundef %64)
  %65 = load i32, ptr %1, align 4
  %66 = call i32 @lsli0(i32 noundef %65)
  %67 = zext i32 %66 to i64
  call void @pll(i64 noundef %67)
  %68 = load i32, ptr %1, align 4
  %69 = call i32 @lsri0(i32 noundef %68)
  %70 = zext i32 %69 to i64
  call void @pll(i64 noundef %70)
  %71 = load i32, ptr %1, align 4
  %72 = sext i32 %71 to i64
  %73 = call i64 @asrl0(i64 noundef %72)
  call void @pll(i64 noundef %73)
  %74 = load i32, ptr %1, align 4
  %75 = call i32 @lsli1(i32 noundef %74)
  %76 = zext i32 %75 to i64
  call void @pll(i64 noundef %76)
  %77 = load i32, ptr %1, align 4
  %78 = call i32 @lsli31(i32 noundef %77)
  %79 = zext i32 %78 to i64
  call void @pll(i64 noundef %79)
  %80 = load i32, ptr %1, align 4
  %81 = sext i32 %80 to i64
  %82 = call i64 @lsll1(i64 noundef %81)
  call void @pll(i64 noundef %82)
  %83 = load i32, ptr %1, align 4
  %84 = sext i32 %83 to i64
  %85 = call i64 @lsll63(i64 noundef %84)
  call void @pll(i64 noundef %85)
  %86 = load i32, ptr %1, align 4
  %87 = call i32 @lsri1(i32 noundef %86)
  %88 = zext i32 %87 to i64
  call void @pll(i64 noundef %88)
  %89 = load i32, ptr %1, align 4
  %90 = call i32 @lsri31(i32 noundef %89)
  %91 = zext i32 %90 to i64
  call void @pll(i64 noundef %91)
  %92 = load i32, ptr %1, align 4
  %93 = sext i32 %92 to i64
  %94 = call i64 @lsrl1(i64 noundef %93)
  call void @pll(i64 noundef %94)
  %95 = load i32, ptr %1, align 4
  %96 = sext i32 %95 to i64
  %97 = call i64 @lsrl63(i64 noundef %96)
  call void @pll(i64 noundef %97)
  %98 = load i32, ptr %1, align 4
  %99 = call i32 @asri1(i32 noundef %98)
  %100 = sext i32 %99 to i64
  call void @pll(i64 noundef %100)
  %101 = load i32, ptr %1, align 4
  %102 = call i32 @asri31(i32 noundef %101)
  %103 = sext i32 %102 to i64
  call void @pll(i64 noundef %103)
  %104 = load i32, ptr %1, align 4
  %105 = sext i32 %104 to i64
  %106 = call i64 @asrl1(i64 noundef %105)
  call void @pll(i64 noundef %106)
  %107 = load i32, ptr %1, align 4
  %108 = sext i32 %107 to i64
  %109 = call i64 @asrl63(i64 noundef %108)
  call void @pll(i64 noundef %109)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @addip0(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = add i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @sublp0(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = sub i64 %3, 0
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @addip123(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = add i32 %3, 123
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @addlm123(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = add i64 %3, -123
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @sublp4095(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = sub i64 %3, 4095
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @subim503808(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = sub i32 %3, -503808
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @addp12345(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = add i64 %3, 12345
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @subp12345(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = sub i32 %3, 12345
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @mvni(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = sub i32 -1, %3
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @negl(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = sub i64 0, %3
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @rsbi123(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = sub i32 123, %3
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @rsbl123(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = sub i64 123, %3
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @andi0(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = and i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @andlm1(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = and i64 %3, -1
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @orrl0(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = or i64 %3, 0
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @orrim1(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = or i32 %3, -1
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @eori0(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = xor i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @eorlm1(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = xor i64 %3, -1
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @and0xf0(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = and i32 %3, 240
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @orr0xf0(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = or i64 %3, 240
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @eor0xf0(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = xor i64 %3, 240
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @lsli0(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = shl i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @lsri0(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = lshr i32 %3, 0
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @asrl0(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = ashr i64 %3, 0
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @lsli1(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = shl i32 %3, 1
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @lsli31(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = shl i32 %3, 31
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @lsll1(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = shl i64 %3, 1
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @lsll63(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = shl i64 %3, 63
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @lsri1(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = lshr i32 %3, 1
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @lsri31(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = lshr i32 %3, 31
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @lsrl1(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = lshr i64 %3, 1
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @lsrl63(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = lshr i64 %3, 63
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @asri1(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = ashr i32 %3, 1
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i32 @asri31(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = ashr i32 %3, 31
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @asrl1(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = ashr i64 %3, 1
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define internal i64 @asrl63(i64 noundef %0) #0 {
  %2 = alloca i64, align 8
  store i64 %0, ptr %2, align 8
  %3 = load i64, ptr %2, align 8
  %4 = ashr i64 %3, 63
  ret i64 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @pcs() #0 {
  call void @arg()
  call void @ret()
  call void @stdarg()
  call void @movi()
  call void @opi()
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  call void @pcs()
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="64" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nocallback nofree nosync nounwind willreturn }
attributes #5 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
