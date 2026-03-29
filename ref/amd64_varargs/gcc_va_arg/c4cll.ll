target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)

declare i32 @exit(...)
declare i32 @abort(...)

define i32 @f(i64 %p.p0, i64 %p.p1, i64 %p.p2, i64 %p.p3, i64 %p.p4, i64 %p.p5, i64 %p.p6, i64 %p.p7, i64 %p.p8, ...)
{
entry:
  %lv.select = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.select)
  %t0 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 3
  %t1 = load i32, ptr %t0
  %t6 = icmp sge i32 %t1, 0
  br i1 %t6, label %vaarg.stack.2, label %vaarg.regtry.3
vaarg.regtry.3:
  %t7 = add i32 %t1, 8
  store i32 %t7, ptr %t0
  %t8 = icmp sle i32 %t7, 0
  br i1 %t8, label %vaarg.reg.4, label %vaarg.stack.2
vaarg.reg.4:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 1
  %t10 = load ptr, ptr %t9
  %t11 = getelementptr i8, ptr %t10, i32 %t1
  br label %vaarg.join.5
vaarg.stack.2:
  %t12 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 0
  %t13 = load ptr, ptr %t12
  %t14 = getelementptr i8, ptr %t13, i64 8
  store ptr %t14, ptr %t12
  br label %vaarg.join.5
vaarg.join.5:
  %t15 = phi ptr [ %t11, %vaarg.reg.4 ], [ %t13, %vaarg.stack.2 ]
  %t16 = load i64, ptr %t15
  %t17 = sext i32 10 to i64
  %t18 = icmp ne i64 %t16, %t17
  %t19 = zext i1 %t18 to i32
  %t20 = icmp ne i32 %t19, 0
  br i1 %t20, label %block_1, label %block_2
block_1:
  %t21 = call i32 @abort()
  br label %block_2
block_2:
  %t22 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 3
  %t23 = load i32, ptr %t22
  %t28 = icmp sge i32 %t23, 0
  br i1 %t28, label %vaarg.stack.24, label %vaarg.regtry.25
vaarg.regtry.25:
  %t29 = add i32 %t23, 8
  store i32 %t29, ptr %t22
  %t30 = icmp sle i32 %t29, 0
  br i1 %t30, label %vaarg.reg.26, label %vaarg.stack.24
vaarg.reg.26:
  %t31 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 1
  %t32 = load ptr, ptr %t31
  %t33 = getelementptr i8, ptr %t32, i32 %t23
  br label %vaarg.join.27
vaarg.stack.24:
  %t34 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 0
  %t35 = load ptr, ptr %t34
  %t36 = getelementptr i8, ptr %t35, i64 8
  store ptr %t36, ptr %t34
  br label %vaarg.join.27
vaarg.join.27:
  %t37 = phi ptr [ %t33, %vaarg.reg.26 ], [ %t35, %vaarg.stack.24 ]
  %t38 = load i64, ptr %t37
  %t39 = sext i32 11 to i64
  %t40 = icmp ne i64 %t38, %t39
  %t41 = zext i1 %t40 to i32
  %t42 = icmp ne i32 %t41, 0
  br i1 %t42, label %block_3, label %block_4
block_3:
  %t43 = call i32 @abort()
  br label %block_4
block_4:
  %t44 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 3
  %t45 = load i32, ptr %t44
  %t50 = icmp sge i32 %t45, 0
  br i1 %t50, label %vaarg.stack.46, label %vaarg.regtry.47
vaarg.regtry.47:
  %t51 = add i32 %t45, 8
  store i32 %t51, ptr %t44
  %t52 = icmp sle i32 %t51, 0
  br i1 %t52, label %vaarg.reg.48, label %vaarg.stack.46
vaarg.reg.48:
  %t53 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 1
  %t54 = load ptr, ptr %t53
  %t55 = getelementptr i8, ptr %t54, i32 %t45
  br label %vaarg.join.49
vaarg.stack.46:
  %t56 = getelementptr %struct.__va_list_tag_, ptr %lv.select, i32 0, i32 0
  %t57 = load ptr, ptr %t56
  %t58 = getelementptr i8, ptr %t57, i64 8
  store ptr %t58, ptr %t56
  br label %vaarg.join.49
vaarg.join.49:
  %t59 = phi ptr [ %t55, %vaarg.reg.48 ], [ %t57, %vaarg.stack.46 ]
  %t60 = load i64, ptr %t59
  %t61 = sext i32 0 to i64
  %t62 = icmp ne i64 %t60, %t61
  %t63 = zext i1 %t62 to i32
  %t64 = icmp ne i32 %t63, 0
  br i1 %t64, label %block_5, label %block_6
block_5:
  %t65 = call i32 @abort()
  br label %block_6
block_6:
  call void @llvm.va_end.p0(ptr %lv.select)
  ret i32 0
}

define i32 @main()
{
entry:
  %t0 = call i32 (i64, i64, i64, i64, i64, i64, i64, i64, i64, ...) @f(i64 1, i64 2, i64 3, i64 4, i64 5, i64 6, i64 7, i64 8, i64 9, i64 10, i64 11, i64 0)
  %t1 = call i32 @exit(i32 0)
  ret i32 0
}

