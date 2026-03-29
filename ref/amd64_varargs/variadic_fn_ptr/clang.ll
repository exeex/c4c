; ModuleID = 'tests/c/internal/positive_case/ok_fn_returns_variadic_fn_ptr.c'
source_filename = "tests/c/internal/positive_case/ok_fn_returns_variadic_fn_ptr.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.__va_list_tag = type { i32, i32, ptr, ptr }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @sum2(i32 noundef %0, ...) #0 {
  %2 = alloca i32, align 4
  %3 = alloca [1 x %struct.__va_list_tag], align 16
  %4 = alloca i32, align 4
  store i32 %0, ptr %2, align 4
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_start.p0(ptr %5)
  %6 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  %7 = getelementptr inbounds %struct.__va_list_tag, ptr %6, i32 0, i32 0
  %8 = load i32, ptr %7, align 16
  %9 = icmp ule i32 %8, 40
  br i1 %9, label %10, label %15

10:                                               ; preds = %1
  %11 = getelementptr inbounds %struct.__va_list_tag, ptr %6, i32 0, i32 3
  %12 = load ptr, ptr %11, align 16
  %13 = getelementptr i8, ptr %12, i32 %8
  %14 = add i32 %8, 8
  store i32 %14, ptr %7, align 16
  br label %19

15:                                               ; preds = %1
  %16 = getelementptr inbounds %struct.__va_list_tag, ptr %6, i32 0, i32 2
  %17 = load ptr, ptr %16, align 8
  %18 = getelementptr i8, ptr %17, i32 8
  store ptr %18, ptr %16, align 8
  br label %19

19:                                               ; preds = %15, %10
  %20 = phi ptr [ %13, %10 ], [ %17, %15 ]
  %21 = load i32, ptr %20, align 4
  store i32 %21, ptr %4, align 4
  %22 = getelementptr inbounds [1 x %struct.__va_list_tag], ptr %3, i64 0, i64 0
  call void @llvm.va_end.p0(ptr %22)
  %23 = load i32, ptr %2, align 4
  %24 = load i32, ptr %4, align 4
  %25 = add nsw i32 %23, %24
  ret i32 %25
}

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_start.p0(ptr) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn
declare void @llvm.va_end.p0(ptr) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local ptr @pickv() #0 {
  ret ptr @sum2
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = call ptr @pickv()
  %3 = call i32 (i32, ...) %2(i32 noundef 10, i32 noundef 32)
  %4 = icmp eq i32 %3, 42
  %5 = zext i1 %4 to i64
  %6 = select i1 %4, i32 0, i32 1
  ret i32 %6
}

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind willreturn }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
