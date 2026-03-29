; ModuleID = 'tests/c/external/gcc_torture/src/strct-stdarg-1.c'
source_filename = "tests/c/external/gcc_torture/src/strct-stdarg-1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.tiny = type { i8, i8, i8, i8, i8 }
%struct.__va_list_tag = type { i32, i32, ptr, ptr }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @f(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca %struct.tiny, align 1
  %5 = alloca i32, align 4
  %6 = alloca [1 x %struct.__va_list_tag], align 16
  %7 = alloca i64, align 8
  store i32 %0, ptr %3, align 4
  %8 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %6, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %8)
  store i32 0, ptr %5, align 4
  br label %9

9:                                                ; preds = %69, %1
  %10 = load i32, ptr %5, align 4
  %11 = load i32, ptr %3, align 4
  %12 = icmp slt i32 %10, %11
  br i1 %12, label %13, label %72

13:                                               ; preds = %9
  %14 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %6, i64 0, i64 0
  %15 = getelementptr inbounds %struct.__va_list_tag, ptr %14, i32 0, i32 0
  %16 = load i32, ptr %15, align 16
  %17 = icmp ule i32 %16, 40
  br i1 %17, label %18, label %23

18:                                               ; preds = %13
  %19 = getelementptr inbounds %struct.__va_list_tag, ptr %14, i32 0, i32 3
  %20 = load ptr, ptr %19, align 16
  %21 = getelementptr i8, ptr %20, i32 %16
  %22 = add i32 %16, 8
  store i32 %22, ptr %15, align 16
  br label %27

23:                                               ; preds = %13
  %24 = getelementptr inbounds %struct.__va_list_tag, ptr %14, i32 0, i32 2
  %25 = load ptr, ptr %24, align 8
  %26 = getelementptr i8, ptr %25, i32 8
  store ptr %26, ptr %24, align 8
  br label %27

27:                                               ; preds = %23, %18
  %28 = phi ptr [ %21, %18 ], [ %25, %23 ]
  call void @llvm.memcpy.p0.p0.i64(ptr align 1 %4, ptr align 1 %28, i64 5, i1 false)
  %29 = getelementptr inbounds %struct.tiny, ptr %4, i32 0, i32 0
  %30 = load i8, ptr %29, align 1
  %31 = sext i8 %30 to i32
  %32 = load i32, ptr %5, align 4
  %33 = add nsw i32 %32, 10
  %34 = icmp ne i32 %31, %33
  br i1 %34, label %35, label %36

35:                                               ; preds = %27
  call void @abort() #5
  unreachable

36:                                               ; preds = %27
  %37 = getelementptr inbounds %struct.tiny, ptr %4, i32 0, i32 1
  %38 = load i8, ptr %37, align 1
  %39 = sext i8 %38 to i32
  %40 = load i32, ptr %5, align 4
  %41 = add nsw i32 %40, 20
  %42 = icmp ne i32 %39, %41
  br i1 %42, label %43, label %44

43:                                               ; preds = %36
  call void @abort() #5
  unreachable

44:                                               ; preds = %36
  %45 = getelementptr inbounds %struct.tiny, ptr %4, i32 0, i32 2
  %46 = load i8, ptr %45, align 1
  %47 = sext i8 %46 to i32
  %48 = load i32, ptr %5, align 4
  %49 = add nsw i32 %48, 30
  %50 = icmp ne i32 %47, %49
  br i1 %50, label %51, label %52

51:                                               ; preds = %44
  call void @abort() #5
  unreachable

52:                                               ; preds = %44
  %53 = getelementptr inbounds %struct.tiny, ptr %4, i32 0, i32 3
  %54 = load i8, ptr %53, align 1
  %55 = sext i8 %54 to i32
  %56 = load i32, ptr %5, align 4
  %57 = add nsw i32 %56, 40
  %58 = icmp ne i32 %55, %57
  br i1 %58, label %59, label %60

59:                                               ; preds = %52
  call void @abort() #5
  unreachable

60:                                               ; preds = %52
  %61 = getelementptr inbounds %struct.tiny, ptr %4, i32 0, i32 4
  %62 = load i8, ptr %61, align 1
  %63 = sext i8 %62 to i32
  %64 = load i32, ptr %5, align 4
  %65 = add nsw i32 %64, 50
  %66 = icmp ne i32 %63, %65
  br i1 %66, label %67, label %68

67:                                               ; preds = %60
  call void @abort() #5
  unreachable

68:                                               ; preds = %60
  br label %69

69:                                               ; preds = %68
  %70 = load i32, ptr %5, align 4
  %71 = add nsw i32 %70, 1
  store i32 %71, ptr %5, align 4
  br label %9

72:                                               ; preds = %9
  %73 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %6, i64 0, i64 0
  %74 = getelementptr inbounds %struct.__va_list_tag, ptr %73, i32 0, i32 0
  %75 = load i32, ptr %74, align 16
  %76 = icmp ule i32 %75, 40
  br i1 %76, label %77, label %82

77:                                               ; preds = %72
  %78 = getelementptr inbounds %struct.__va_list_tag, ptr %73, i32 0, i32 3
  %79 = load ptr, ptr %78, align 16
  %80 = getelementptr i8, ptr %79, i32 %75
  %81 = add i32 %75, 8
  store i32 %81, ptr %74, align 16
  br label %86

82:                                               ; preds = %72
  %83 = getelementptr inbounds %struct.__va_list_tag, ptr %73, i32 0, i32 2
  %84 = load ptr, ptr %83, align 8
  %85 = getelementptr i8, ptr %84, i32 8
  store ptr %85, ptr %83, align 8
  br label %86

86:                                               ; preds = %82, %77
  %87 = phi ptr [ %80, %77 ], [ %84, %82 ]
  %88 = load i64, ptr %87, align 8
  store i64 %88, ptr %7, align 8
  %89 = load i64, ptr %7, align 8
  %90 = icmp ne i64 %89, 123
  br i1 %90, label %91, label %92

91:                                               ; preds = %86
  call void @abort() #5
  unreachable

92:                                               ; preds = %86
  %93 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %6, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %93)
  %94 = load i32, ptr %2, align 4
  ret i32 %94
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #1

; Function Attrs: nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #2

; Function Attrs: noreturn nounwind
declare void @abort() #3

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p0(ptr) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [3 x %struct.tiny], align 1
  %3 = alloca i40, align 8
  %4 = alloca i40, align 8
  %5 = alloca i40, align 8
  store i32 0, ptr %1, align 4
  %6 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 0
  %7 = getelementptr inbounds %struct.tiny, ptr %6, i32 0, i32 0
  store i8 10, ptr %7, align 1
  %8 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 1
  %9 = getelementptr inbounds %struct.tiny, ptr %8, i32 0, i32 0
  store i8 11, ptr %9, align 1
  %10 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 2
  %11 = getelementptr inbounds %struct.tiny, ptr %10, i32 0, i32 0
  store i8 12, ptr %11, align 1
  %12 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 0
  %13 = getelementptr inbounds %struct.tiny, ptr %12, i32 0, i32 1
  store i8 20, ptr %13, align 1
  %14 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 1
  %15 = getelementptr inbounds %struct.tiny, ptr %14, i32 0, i32 1
  store i8 21, ptr %15, align 1
  %16 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 2
  %17 = getelementptr inbounds %struct.tiny, ptr %16, i32 0, i32 1
  store i8 22, ptr %17, align 1
  %18 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 0
  %19 = getelementptr inbounds %struct.tiny, ptr %18, i32 0, i32 2
  store i8 30, ptr %19, align 1
  %20 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 1
  %21 = getelementptr inbounds %struct.tiny, ptr %20, i32 0, i32 2
  store i8 31, ptr %21, align 1
  %22 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 2
  %23 = getelementptr inbounds %struct.tiny, ptr %22, i32 0, i32 2
  store i8 32, ptr %23, align 1
  %24 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 0
  %25 = getelementptr inbounds %struct.tiny, ptr %24, i32 0, i32 3
  store i8 40, ptr %25, align 1
  %26 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 1
  %27 = getelementptr inbounds %struct.tiny, ptr %26, i32 0, i32 3
  store i8 41, ptr %27, align 1
  %28 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 2
  %29 = getelementptr inbounds %struct.tiny, ptr %28, i32 0, i32 3
  store i8 42, ptr %29, align 1
  %30 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 0
  %31 = getelementptr inbounds %struct.tiny, ptr %30, i32 0, i32 4
  store i8 50, ptr %31, align 1
  %32 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 1
  %33 = getelementptr inbounds %struct.tiny, ptr %32, i32 0, i32 4
  store i8 51, ptr %33, align 1
  %34 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 2
  %35 = getelementptr inbounds %struct.tiny, ptr %34, i32 0, i32 4
  store i8 52, ptr %35, align 1
  %36 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 0
  %37 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 1
  %38 = getelementptr inbounds [3 x %struct.tiny], ptr %2, i64 0, i64 2
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %3, ptr align 1 %36, i64 5, i1 false)
  %39 = load i40, ptr %3, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %4, ptr align 1 %37, i64 5, i1 false)
  %40 = load i40, ptr %4, align 8
  call void @llvm.memcpy.p0.p0.i64(ptr align 8 %5, ptr align 1 %38, i64 5, i1 false)
  %41 = load i40, ptr %5, align 8
  %42 = call i32 (i32, ...) @f(i32 noundef 3, i40 %39, i40 %40, i40 %41, i64 noundef 123)
  call void @exit(i32 noundef 0) #6
  unreachable
}

; Function Attrs: noreturn
declare void @exit(i32 noundef) #4

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind willreturn }
attributes #2 = { nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { noreturn "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { noreturn nounwind }
attributes #6 = { noreturn }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
