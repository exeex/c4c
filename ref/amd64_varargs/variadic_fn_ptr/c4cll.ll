target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)

define i32 @sum2(i32 %p.first, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  %lv.second = alloca i32, align 4
  call void @llvm.va_start.p0(ptr %lv.ap)
  %t0 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t1 = load i32, ptr %t0
  %t6 = icmp sge i32 %t1, 0
  br i1 %t6, label %vaarg.stack.2, label %vaarg.regtry.3
vaarg.regtry.3:
  %t7 = add i32 %t1, 8
  store i32 %t7, ptr %t0
  %t8 = icmp sle i32 %t7, 0
  br i1 %t8, label %vaarg.reg.4, label %vaarg.stack.2
vaarg.reg.4:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t10 = load ptr, ptr %t9
  %t11 = getelementptr i8, ptr %t10, i32 %t1
  br label %vaarg.join.5
vaarg.stack.2:
  %t12 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t13 = load ptr, ptr %t12
  %t14 = getelementptr i8, ptr %t13, i64 8
  store ptr %t14, ptr %t12
  br label %vaarg.join.5
vaarg.join.5:
  %t15 = phi ptr [ %t11, %vaarg.reg.4 ], [ %t13, %vaarg.stack.2 ]
  %t16 = load i32, ptr %t15
  store i32 %t16, ptr %lv.second
  call void @llvm.va_end.p0(ptr %lv.ap)
  %t17 = load i32, ptr %lv.second
  %t18 = add i32 %p.first, %t17
  ret i32 %t18
}

define ptr @pickv()
{
entry:
  ret ptr @sum2
}

define i32 @main()
{
entry:
  %t0 = call ptr () @pickv()
  %t1 = call i32 %t0(i32 10, i32 32)
  %t2 = icmp eq i32 %t1, 42
  %t3 = zext i1 %t2 to i32
  %t4 = icmp ne i32 %t3, 0
  br i1 %t4, label %tern.then.5, label %tern.else.7
tern.then.5:
  br label %tern.then.end.6
tern.then.end.6:
  br label %tern.end.9
tern.else.7:
  br label %tern.else.end.8
tern.else.end.8:
  br label %tern.end.9
tern.end.9:
  %t10 = phi i32 [ 0, %tern.then.end.6 ], [ 1, %tern.else.end.8 ]
  ret i32 %t10
}

