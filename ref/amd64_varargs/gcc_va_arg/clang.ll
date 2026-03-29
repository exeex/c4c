; ModuleID = 'tests/c/external/gcc_torture/src/va-arg-1.c'
source_filename = "tests/c/external/gcc_torture/src/va-arg-1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.__va_list_tag = type { i32, i32, ptr, ptr }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @f(i64 noundef %0, i64 noundef %1, i64 noundef %2, i64 noundef %3, i64 noundef %4, i64 noundef %5, i64 noundef %6, i64 noundef %7, i64 noundef %8, ...) #0 {
  %10 = alloca i32, align 4
  %11 = alloca i64, align 8
  %12 = alloca i64, align 8
  %13 = alloca i64, align 8
  %14 = alloca i64, align 8
  %15 = alloca i64, align 8
  %16 = alloca i64, align 8
  %17 = alloca i64, align 8
  %18 = alloca i64, align 8
  %19 = alloca i64, align 8
  %20 = alloca [1 x %struct.__va_list_tag], align 16
  store i64 %0, ptr %11, align 8
  store i64 %1, ptr %12, align 8
  store i64 %2, ptr %13, align 8
  store i64 %3, ptr %14, align 8
  store i64 %4, ptr %15, align 8
  store i64 %5, ptr %16, align 8
  store i64 %6, ptr %17, align 8
  store i64 %7, ptr %18, align 8
  store i64 %8, ptr %19, align 8
  %21 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %20, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %21)
  %22 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %20, i64 0, i64 0
  %23 = getelementptr inbounds %struct.__va_list_tag, ptr %22, i32 0, i32 0
  %24 = load i32, ptr %23, align 16
  %25 = icmp ule i32 %24, 40
  br i1 %25, label %26, label %31

26:                                               ; preds = %9
  %27 = getelementptr inbounds %struct.__va_list_tag, ptr %22, i32 0, i32 3
  %28 = load ptr, ptr %27, align 16
  %29 = getelementptr i8, ptr %28, i32 %24
  %30 = add i32 %24, 8
  store i32 %30, ptr %23, align 16
  br label %35

31:                                               ; preds = %9
  %32 = getelementptr inbounds %struct.__va_list_tag, ptr %22, i32 0, i32 2
  %33 = load ptr, ptr %32, align 8
  %34 = getelementptr i8, ptr %33, i32 8
  store ptr %34, ptr %32, align 8
  br label %35

35:                                               ; preds = %31, %26
  %36 = phi ptr [ %29, %26 ], [ %33, %31 ]
  %37 = load i64, ptr %36, align 8
  %38 = icmp ne i64 %37, 10
  br i1 %38, label %39, label %40

39:                                               ; preds = %35
  call void @abort() #4
  unreachable

40:                                               ; preds = %35
  %41 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %20, i64 0, i64 0
  %42 = getelementptr inbounds %struct.__va_list_tag, ptr %41, i32 0, i32 0
  %43 = load i32, ptr %42, align 16
  %44 = icmp ule i32 %43, 40
  br i1 %44, label %45, label %50

45:                                               ; preds = %40
  %46 = getelementptr inbounds %struct.__va_list_tag, ptr %41, i32 0, i32 3
  %47 = load ptr, ptr %46, align 16
  %48 = getelementptr i8, ptr %47, i32 %43
  %49 = add i32 %43, 8
  store i32 %49, ptr %42, align 16
  br label %54

50:                                               ; preds = %40
  %51 = getelementptr inbounds %struct.__va_list_tag, ptr %41, i32 0, i32 2
  %52 = load ptr, ptr %51, align 8
  %53 = getelementptr i8, ptr %52, i32 8
  store ptr %53, ptr %51, align 8
  br label %54

54:                                               ; preds = %50, %45
  %55 = phi ptr [ %48, %45 ], [ %52, %50 ]
  %56 = load i64, ptr %55, align 8
  %57 = icmp ne i64 %56, 11
  br i1 %57, label %58, label %59

58:                                               ; preds = %54
  call void @abort() #4
  unreachable

59:                                               ; preds = %54
  %60 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %20, i64 0, i64 0
  %61 = getelementptr inbounds %struct.__va_list_tag, ptr %60, i32 0, i32 0
  %62 = load i32, ptr %61, align 16
  %63 = icmp ule i32 %62, 40
  br i1 %63, label %64, label %69

64:                                               ; preds = %59
  %65 = getelementptr inbounds %struct.__va_list_tag, ptr %60, i32 0, i32 3
  %66 = load ptr, ptr %65, align 16
  %67 = getelementptr i8, ptr %66, i32 %62
  %68 = add i32 %62, 8
  store i32 %68, ptr %61, align 16
  br label %73

69:                                               ; preds = %59
  %70 = getelementptr inbounds %struct.__va_list_tag, ptr %60, i32 0, i32 2
  %71 = load ptr, ptr %70, align 8
  %72 = getelementptr i8, ptr %71, i32 8
  store ptr %72, ptr %70, align 8
  br label %73

73:                                               ; preds = %69, %64
  %74 = phi ptr [ %67, %64 ], [ %71, %69 ]
  %75 = load i64, ptr %74, align 8
  %76 = icmp ne i64 %75, 0
  br i1 %76, label %77, label %78

77:                                               ; preds = %73
  call void @abort() #4
  unreachable

78:                                               ; preds = %73
  %79 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %20, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %79)
  %80 = load i32, ptr %10, align 4
  ret i32 %80
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #1

; Function Attrs: noreturn nounwind
declare void @abort() #2

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p0(ptr) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = call i32 (i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) @f(i64 noundef 1, i64 noundef 2, i64 noundef 3, i64 noundef 4, i64 noundef 5, i64 noundef 6, i64 noundef 7, i64 noundef 8, i64 noundef 9, i64 noundef 10, i64 noundef 11, i64 noundef 0)
  call void @exit(i32 noundef 0) #5
  unreachable
}

; Function Attrs: noreturn
declare void @exit(i32 noundef) #3

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind willreturn }
attributes #2 = { noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { noreturn "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { noreturn nounwind }
attributes #5 = { noreturn }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
