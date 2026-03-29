; ModuleID = 'tests/c/external/gcc_torture/src/stdarg-1.c'
source_filename = "tests/c/external/gcc_torture/src/stdarg-1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.__va_list_tag = type { i32, i32, ptr, ptr }

@foo_arg = dso_local global i32 0, align 4
@gap = dso_local global [1 x %struct.__va_list_tag] zeroinitializer, align 16
@pap = dso_local global ptr null, align 8
@bar_arg = dso_local global i32 0, align 4
@d = dso_local global double 0.000000e+00, align 8
@x = dso_local global i64 0, align 8

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @foo(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca ptr, align 8
  store i32 %0, ptr %3, align 4
  store ptr %1, ptr %4, align 8
  %5 = load i32, ptr %3, align 4
  switch i32 %5, label %23 [
    i32 5, label %6
  ]

6:                                                ; preds = %2
  %7 = load ptr, ptr %4, align 8
  %8 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 0
  %9 = load i32, ptr %8, align 8
  %10 = icmp ule i32 %9, 40
  br i1 %10, label %11, label %16

11:                                               ; preds = %6
  %12 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 3
  %13 = load ptr, ptr %12, align 8
  %14 = getelementptr i8, ptr %13, i32 %9
  %15 = add i32 %9, 8
  store i32 %15, ptr %8, align 8
  br label %20

16:                                               ; preds = %6
  %17 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 2
  %18 = load ptr, ptr %17, align 8
  %19 = getelementptr i8, ptr %18, i32 8
  store ptr %19, ptr %17, align 8
  br label %20

20:                                               ; preds = %16, %11
  %21 = phi ptr [ %14, %11 ], [ %18, %16 ]
  %22 = load i32, ptr %21, align 4
  store i32 %22, ptr @foo_arg, align 4
  br label %24

23:                                               ; preds = %2
  call void @abort() #4
  unreachable

24:                                               ; preds = %20
  ret void
}

; Function Attrs: noreturn nounwind
declare void @abort() #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @bar(i32 noundef %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %3 = load i32, ptr %2, align 4
  %4 = icmp eq i32 %3, 16390
  br i1 %4, label %5, label %35

5:                                                ; preds = %1
  %6 = load i32, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 1), align 4
  %7 = icmp ule i32 %6, 160
  br i1 %7, label %8, label %12

8:                                                ; preds = %5
  %9 = load ptr, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 3), align 16
  %10 = getelementptr i8, ptr %9, i32 %6
  %11 = add i32 %6, 16
  store i32 %11, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 1), align 4
  br label %15

12:                                               ; preds = %5
  %13 = load ptr, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 2), align 8
  %14 = getelementptr i8, ptr %13, i32 8
  store ptr %14, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 2), align 8
  br label %15

15:                                               ; preds = %12, %8
  %16 = phi ptr [ %10, %8 ], [ %13, %12 ]
  %17 = load double, ptr %16, align 8
  %18 = fcmp une double %17, 1.700000e+01
  br i1 %18, label %33, label %19

19:                                               ; preds = %15
  %20 = load i32, ptr @gap, align 16
  %21 = icmp ule i32 %20, 40
  br i1 %21, label %22, label %26

22:                                               ; preds = %19
  %23 = load ptr, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 3), align 16
  %24 = getelementptr i8, ptr %23, i32 %20
  %25 = add i32 %20, 8
  store i32 %25, ptr @gap, align 16
  br label %29

26:                                               ; preds = %19
  %27 = load ptr, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 2), align 8
  %28 = getelementptr i8, ptr %27, i32 8
  store ptr %28, ptr getelementptr inbounds (%struct.__va_list_tag, ptr @gap, i32 0, i32 2), align 8
  br label %29

29:                                               ; preds = %26, %22
  %30 = phi ptr [ %24, %22 ], [ %27, %26 ]
  %31 = load i64, ptr %30, align 8
  %32 = icmp ne i64 %31, 129
  br i1 %32, label %33, label %34

33:                                               ; preds = %29, %15
  call void @abort() #4
  unreachable

34:                                               ; preds = %29
  br label %89

35:                                               ; preds = %1
  %36 = load i32, ptr %2, align 4
  %37 = icmp eq i32 %36, 16392
  br i1 %37, label %38, label %88

38:                                               ; preds = %35
  %39 = load ptr, ptr @pap, align 8
  %40 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %39, i64 0, i64 0
  %41 = getelementptr inbounds %struct.__va_list_tag, ptr %40, i32 0, i32 0
  %42 = load i32, ptr %41, align 8
  %43 = icmp ule i32 %42, 40
  br i1 %43, label %44, label %49

44:                                               ; preds = %38
  %45 = getelementptr inbounds %struct.__va_list_tag, ptr %40, i32 0, i32 3
  %46 = load ptr, ptr %45, align 8
  %47 = getelementptr i8, ptr %46, i32 %42
  %48 = add i32 %42, 8
  store i32 %48, ptr %41, align 8
  br label %53

49:                                               ; preds = %38
  %50 = getelementptr inbounds %struct.__va_list_tag, ptr %40, i32 0, i32 2
  %51 = load ptr, ptr %50, align 8
  %52 = getelementptr i8, ptr %51, i32 8
  store ptr %52, ptr %50, align 8
  br label %53

53:                                               ; preds = %49, %44
  %54 = phi ptr [ %47, %44 ], [ %51, %49 ]
  %55 = load i64, ptr %54, align 8
  %56 = icmp ne i64 %55, 14
  br i1 %56, label %86, label %57

57:                                               ; preds = %53
  %58 = load ptr, ptr @pap, align 8
  %59 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %58, i64 0, i64 0
  %60 = getelementptr inbounds %struct.__va_list_tag, ptr %59, i32 0, i32 2
  %61 = load ptr, ptr %60, align 8
  %62 = getelementptr inbounds i8, ptr %61, i32 15
  %63 = call ptr @llvm.ptrmask.p0.i64(ptr %62, i64 -16)
  %64 = getelementptr i8, ptr %63, i32 16
  store ptr %64, ptr %60, align 8
  %65 = load x86_fp80, ptr %63, align 16
  %66 = fcmp une x86_fp80 %65, 0xK40068300000000000000
  br i1 %66, label %86, label %67

67:                                               ; preds = %57
  %68 = load ptr, ptr @pap, align 8
  %69 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %68, i64 0, i64 0
  %70 = getelementptr inbounds %struct.__va_list_tag, ptr %69, i32 0, i32 0
  %71 = load i32, ptr %70, align 8
  %72 = icmp ule i32 %71, 40
  br i1 %72, label %73, label %78

73:                                               ; preds = %67
  %74 = getelementptr inbounds %struct.__va_list_tag, ptr %69, i32 0, i32 3
  %75 = load ptr, ptr %74, align 8
  %76 = getelementptr i8, ptr %75, i32 %71
  %77 = add i32 %71, 8
  store i32 %77, ptr %70, align 8
  br label %82

78:                                               ; preds = %67
  %79 = getelementptr inbounds %struct.__va_list_tag, ptr %69, i32 0, i32 2
  %80 = load ptr, ptr %79, align 8
  %81 = getelementptr i8, ptr %80, i32 8
  store ptr %81, ptr %79, align 8
  br label %82

82:                                               ; preds = %78, %73
  %83 = phi ptr [ %76, %73 ], [ %80, %78 ]
  %84 = load i32, ptr %83, align 4
  %85 = icmp ne i32 %84, 17
  br i1 %85, label %86, label %87

86:                                               ; preds = %82, %57, %53
  call void @abort() #4
  unreachable

87:                                               ; preds = %82
  br label %88

88:                                               ; preds = %87, %35
  br label %89

89:                                               ; preds = %88, %34
  %90 = load i32, ptr %2, align 4
  store i32 %90, ptr @bar_arg, align 4
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare ptr @llvm.ptrmask.p0.i64(ptr, i64) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f0(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f1(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %5)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #3

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p0(ptr) #3

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f2(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  %5 = load double, ptr @d, align 8
  %6 = fptosi double %5 to i32
  call void @bar(i32 noundef %6)
  %7 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %8 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 0
  %9 = load i32, ptr %8, align 16
  %10 = icmp ule i32 %9, 40
  br i1 %10, label %11, label %16

11:                                               ; preds = %1
  %12 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 3
  %13 = load ptr, ptr %12, align 16
  %14 = getelementptr i8, ptr %13, i32 %9
  %15 = add i32 %9, 8
  store i32 %15, ptr %8, align 16
  br label %20

16:                                               ; preds = %1
  %17 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 2
  %18 = load ptr, ptr %17, align 8
  %19 = getelementptr i8, ptr %18, i32 8
  store ptr %19, ptr %17, align 8
  br label %20

20:                                               ; preds = %16, %11
  %21 = phi ptr [ %14, %11 ], [ %18, %16 ]
  %22 = load i64, ptr %21, align 8
  store i64 %22, ptr @x, align 8
  %23 = load i64, ptr @x, align 8
  %24 = trunc i64 %23 to i32
  call void @bar(i32 noundef %24)
  %25 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %25)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f3(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %6 = getelementptr inbounds %struct.__va_list_tag, ptr %5, i32 0, i32 1
  %7 = load i32, ptr %6, align 4
  %8 = icmp ule i32 %7, 160
  br i1 %8, label %9, label %14

9:                                                ; preds = %1
  %10 = getelementptr inbounds %struct.__va_list_tag, ptr %5, i32 0, i32 3
  %11 = load ptr, ptr %10, align 16
  %12 = getelementptr i8, ptr %11, i32 %7
  %13 = add i32 %7, 16
  store i32 %13, ptr %6, align 4
  br label %18

14:                                               ; preds = %1
  %15 = getelementptr inbounds %struct.__va_list_tag, ptr %5, i32 0, i32 2
  %16 = load ptr, ptr %15, align 8
  %17 = getelementptr i8, ptr %16, i32 8
  store ptr %17, ptr %15, align 8
  br label %18

18:                                               ; preds = %14, %9
  %19 = phi ptr [ %12, %9 ], [ %16, %14 ]
  %20 = load double, ptr %19, align 8
  store double %20, ptr @d, align 8
  %21 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %21)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f4(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %6 = getelementptr inbounds %struct.__va_list_tag, ptr %5, i32 0, i32 1
  %7 = load i32, ptr %6, align 4
  %8 = icmp ule i32 %7, 160
  br i1 %8, label %9, label %14

9:                                                ; preds = %1
  %10 = getelementptr inbounds %struct.__va_list_tag, ptr %5, i32 0, i32 3
  %11 = load ptr, ptr %10, align 16
  %12 = getelementptr i8, ptr %11, i32 %7
  %13 = add i32 %7, 16
  store i32 %13, ptr %6, align 4
  br label %18

14:                                               ; preds = %1
  %15 = getelementptr inbounds %struct.__va_list_tag, ptr %5, i32 0, i32 2
  %16 = load ptr, ptr %15, align 8
  %17 = getelementptr i8, ptr %16, i32 8
  store ptr %17, ptr %15, align 8
  br label %18

18:                                               ; preds = %14, %9
  %19 = phi ptr [ %12, %9 ], [ %16, %14 ]
  %20 = load double, ptr %19, align 8
  %21 = fptosi double %20 to i64
  store i64 %21, ptr @x, align 8
  %22 = load i32, ptr %2, align 4
  %23 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @foo(i32 noundef %22, ptr noundef %23)
  %24 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %24)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f5(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_copy.p0(ptr @gap, ptr %5)
  %6 = load i32, ptr %2, align 4
  call void @bar(i32 noundef %6)
  %7 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %7)
  call void @llvm.va_end.p0(ptr @gap)
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_copy.p0(ptr, ptr) #3

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f6(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  %5 = load double, ptr @d, align 8
  %6 = fptosi double %5 to i32
  call void @bar(i32 noundef %6)
  %7 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %8 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 0
  %9 = load i32, ptr %8, align 16
  %10 = icmp ule i32 %9, 40
  br i1 %10, label %11, label %16

11:                                               ; preds = %1
  %12 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 3
  %13 = load ptr, ptr %12, align 16
  %14 = getelementptr i8, ptr %13, i32 %9
  %15 = add i32 %9, 8
  store i32 %15, ptr %8, align 16
  br label %20

16:                                               ; preds = %1
  %17 = getelementptr inbounds %struct.__va_list_tag, ptr %7, i32 0, i32 2
  %18 = load ptr, ptr %17, align 8
  %19 = getelementptr i8, ptr %18, i32 8
  store ptr %19, ptr %17, align 8
  br label %20

20:                                               ; preds = %16, %11
  %21 = phi ptr [ %14, %11 ], [ %18, %16 ]
  %22 = load i64, ptr %21, align 8
  %23 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %24 = getelementptr inbounds %struct.__va_list_tag, ptr %23, i32 0, i32 0
  %25 = load i32, ptr %24, align 16
  %26 = icmp ule i32 %25, 40
  br i1 %26, label %27, label %32

27:                                               ; preds = %20
  %28 = getelementptr inbounds %struct.__va_list_tag, ptr %23, i32 0, i32 3
  %29 = load ptr, ptr %28, align 16
  %30 = getelementptr i8, ptr %29, i32 %25
  %31 = add i32 %25, 8
  store i32 %31, ptr %24, align 16
  br label %36

32:                                               ; preds = %20
  %33 = getelementptr inbounds %struct.__va_list_tag, ptr %23, i32 0, i32 2
  %34 = load ptr, ptr %33, align 8
  %35 = getelementptr i8, ptr %34, i32 8
  store ptr %35, ptr %33, align 8
  br label %36

36:                                               ; preds = %32, %27
  %37 = phi ptr [ %30, %27 ], [ %34, %32 ]
  %38 = load i64, ptr %37, align 8
  %39 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %40 = getelementptr inbounds %struct.__va_list_tag, ptr %39, i32 0, i32 0
  %41 = load i32, ptr %40, align 16
  %42 = icmp ule i32 %41, 40
  br i1 %42, label %43, label %48

43:                                               ; preds = %36
  %44 = getelementptr inbounds %struct.__va_list_tag, ptr %39, i32 0, i32 3
  %45 = load ptr, ptr %44, align 16
  %46 = getelementptr i8, ptr %45, i32 %41
  %47 = add i32 %41, 8
  store i32 %47, ptr %40, align 16
  br label %52

48:                                               ; preds = %36
  %49 = getelementptr inbounds %struct.__va_list_tag, ptr %39, i32 0, i32 2
  %50 = load ptr, ptr %49, align 8
  %51 = getelementptr i8, ptr %50, i32 8
  store ptr %51, ptr %49, align 8
  br label %52

52:                                               ; preds = %48, %43
  %53 = phi ptr [ %46, %43 ], [ %50, %48 ]
  %54 = load i64, ptr %53, align 8
  store i64 %54, ptr @x, align 8
  %55 = load i64, ptr @x, align 8
  %56 = trunc i64 %55 to i32
  call void @bar(i32 noundef %56)
  %57 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %57)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f7(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  store ptr %3, ptr @pap, align 8
  %5 = load i32, ptr %2, align 4
  call void @bar(i32 noundef %5)
  %6 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %6)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f8(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  store i32 %0, ptr %2, align 4
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %4)
  store ptr %3, ptr @pap, align 8
  %5 = load i32, ptr %2, align 4
  call void @bar(i32 noundef %5)
  %6 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %7 = getelementptr inbounds %struct.__va_list_tag, ptr %6, i32 0, i32 1
  %8 = load i32, ptr %7, align 4
  %9 = icmp ule i32 %8, 160
  br i1 %9, label %10, label %15

10:                                               ; preds = %1
  %11 = getelementptr inbounds %struct.__va_list_tag, ptr %6, i32 0, i32 3
  %12 = load ptr, ptr %11, align 16
  %13 = getelementptr i8, ptr %12, i32 %8
  %14 = add i32 %8, 16
  store i32 %14, ptr %7, align 4
  br label %19

15:                                               ; preds = %1
  %16 = getelementptr inbounds %struct.__va_list_tag, ptr %6, i32 0, i32 2
  %17 = load ptr, ptr %16, align 8
  %18 = getelementptr i8, ptr %17, i32 8
  store ptr %18, ptr %16, align 8
  br label %19

19:                                               ; preds = %15, %10
  %20 = phi ptr [ %13, %10 ], [ %17, %15 ]
  %21 = load double, ptr %20, align 8
  store double %21, ptr @d, align 8
  %22 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %22)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  call void (i32, ...) @f0(i32 noundef 1)
  call void (i32, ...) @f1(i32 noundef 2)
  store double 3.100000e+01, ptr @d, align 8
  call void (i32, ...) @f2(i32 noundef 3, i64 noundef 28)
  %2 = load i32, ptr @bar_arg, align 4
  %3 = icmp ne i32 %2, 28
  br i1 %3, label %7, label %4

4:                                                ; preds = %0
  %5 = load i64, ptr @x, align 8
  %6 = icmp ne i64 %5, 28
  br i1 %6, label %7, label %8

7:                                                ; preds = %4, %0
  call void @abort() #4
  unreachable

8:                                                ; preds = %4
  call void (i32, ...) @f3(i32 noundef 4, double noundef 1.310000e+02)
  %9 = load double, ptr @d, align 8
  %10 = fcmp une double %9, 1.310000e+02
  br i1 %10, label %11, label %12

11:                                               ; preds = %8
  call void @abort() #4
  unreachable

12:                                               ; preds = %8
  call void (i32, ...) @f4(i32 noundef 5, double noundef 1.600000e+01, i32 noundef 128)
  %13 = load i64, ptr @x, align 8
  %14 = icmp ne i64 %13, 16
  br i1 %14, label %18, label %15

15:                                               ; preds = %12
  %16 = load i32, ptr @foo_arg, align 4
  %17 = icmp ne i32 %16, 128
  br i1 %17, label %18, label %19

18:                                               ; preds = %15, %12
  call void @abort() #4
  unreachable

19:                                               ; preds = %15
  call void (i32, ...) @f5(i32 noundef 16390, double noundef 1.700000e+01, i64 noundef 129)
  %20 = load i32, ptr @bar_arg, align 4
  %21 = icmp ne i32 %20, 16390
  br i1 %21, label %22, label %23

22:                                               ; preds = %19
  call void @abort() #4
  unreachable

23:                                               ; preds = %19
  call void (i32, ...) @f6(i32 noundef 7, i64 noundef 12, i64 noundef 14, i64 noundef -31)
  %24 = load i32, ptr @bar_arg, align 4
  %25 = icmp ne i32 %24, -31
  br i1 %25, label %26, label %27

26:                                               ; preds = %23
  call void @abort() #4
  unreachable

27:                                               ; preds = %23
  call void (i32, ...) @f7(i32 noundef 16392, i64 noundef 14, x86_fp80 noundef 0xK40068300000000000000, i32 noundef 17, double noundef 2.600000e+01)
  %28 = load i32, ptr @bar_arg, align 4
  %29 = icmp ne i32 %28, 16392
  br i1 %29, label %30, label %31

30:                                               ; preds = %27
  call void @abort() #4
  unreachable

31:                                               ; preds = %27
  call void (i32, ...) @f8(i32 noundef 16392, i64 noundef 14, x86_fp80 noundef 0xK40068300000000000000, i32 noundef 17, double noundef 2.700000e+01)
  %32 = load i32, ptr @bar_arg, align 4
  %33 = icmp ne i32 %32, 16392
  br i1 %33, label %37, label %34

34:                                               ; preds = %31
  %35 = load double, ptr @d, align 8
  %36 = fcmp une double %35, 2.700000e+01
  br i1 %36, label %37, label %38

37:                                               ; preds = %34, %31
  call void @abort() #4
  unreachable

38:                                               ; preds = %34
  ret i32 0
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #3 = { nocallback nofree nosync nounwind willreturn }
attributes #4 = { noreturn nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
