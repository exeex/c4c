target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }
%struct.tiny = type { i8, i8, i8, i8, i8 }

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

declare i32 @exit(...)
declare i32 @abort(...)

define i32 @f(i32 %p.n, ...)
{
entry:
  %lv.x = alloca %struct.tiny, align 1
  %lv.i = alloca i32, align 4
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  %lv.x.1 = alloca i64, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  store i32 0, ptr %lv.i
  br label %for.cond.1
for.cond.1:
  %t0 = load i32, ptr %lv.i
  %t1 = icmp slt i32 %t0, %p.n
  %t2 = zext i1 %t1 to i32
  %t3 = icmp ne i32 %t2, 0
  br i1 %t3, label %block_1, label %block_2
for.latch.1:
  %t4 = load i32, ptr %lv.i
  %t5 = add i32 %t4, 1
  store i32 %t5, ptr %lv.i
  br label %for.cond.1
block_1:
  %t6 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t7 = load i32, ptr %t6
  %t12 = icmp sge i32 %t7, 0
  br i1 %t12, label %vaarg.stack.8, label %vaarg.regtry.9
vaarg.regtry.9:
  %t13 = add i32 %t7, 8
  store i32 %t13, ptr %t6
  %t14 = icmp sle i32 %t13, 0
  br i1 %t14, label %vaarg.reg.10, label %vaarg.stack.8
vaarg.reg.10:
  %t15 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t16 = load ptr, ptr %t15
  %t17 = getelementptr i8, ptr %t16, i32 %t7
  br label %vaarg.join.11
vaarg.stack.8:
  %t18 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t19 = load ptr, ptr %t18
  %t20 = getelementptr i8, ptr %t19, i64 8
  store ptr %t20, ptr %t18
  br label %vaarg.join.11
vaarg.join.11:
  %t21 = phi ptr [ %t17, %vaarg.reg.10 ], [ %t19, %vaarg.stack.8 ]
  %t22 = alloca %struct.tiny
  call void @llvm.memcpy.p0.p0.i64(ptr %t22, ptr %t21, i64 5, i1 false)
  %t23 = load %struct.tiny, ptr %t22
  store %struct.tiny %t23, ptr %lv.x
  %t24 = getelementptr %struct.tiny, ptr %lv.x, i32 0, i32 0
  %t25 = load i8, ptr %t24
  %t26 = load i32, ptr %lv.i
  %t27 = add i32 %t26, 10
  %t28 = sext i8 %t25 to i32
  %t29 = icmp ne i32 %t28, %t27
  %t30 = zext i1 %t29 to i32
  %t31 = icmp ne i32 %t30, 0
  br i1 %t31, label %block_3, label %block_4
block_2:
  %t32 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t33 = load i32, ptr %t32
  %t38 = icmp sge i32 %t33, 0
  br i1 %t38, label %vaarg.stack.34, label %vaarg.regtry.35
vaarg.regtry.35:
  %t39 = add i32 %t33, 8
  store i32 %t39, ptr %t32
  %t40 = icmp sle i32 %t39, 0
  br i1 %t40, label %vaarg.reg.36, label %vaarg.stack.34
vaarg.reg.36:
  %t41 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t42 = load ptr, ptr %t41
  %t43 = getelementptr i8, ptr %t42, i32 %t33
  br label %vaarg.join.37
vaarg.stack.34:
  %t44 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t45 = load ptr, ptr %t44
  %t46 = getelementptr i8, ptr %t45, i64 8
  store ptr %t46, ptr %t44
  br label %vaarg.join.37
vaarg.join.37:
  %t47 = phi ptr [ %t43, %vaarg.reg.36 ], [ %t45, %vaarg.stack.34 ]
  %t48 = load i64, ptr %t47
  store i64 %t48, ptr %lv.x.1
  %t49 = load i64, ptr %lv.x.1
  %t50 = sext i32 123 to i64
  %t51 = icmp ne i64 %t49, %t50
  %t52 = zext i1 %t51 to i32
  %t53 = icmp ne i32 %t52, 0
  br i1 %t53, label %block_13, label %block_14
block_3:
  %t54 = call i32 @abort()
  br label %block_4
block_4:
  %t55 = getelementptr %struct.tiny, ptr %lv.x, i32 0, i32 1
  %t56 = load i8, ptr %t55
  %t57 = load i32, ptr %lv.i
  %t58 = add i32 %t57, 20
  %t59 = sext i8 %t56 to i32
  %t60 = icmp ne i32 %t59, %t58
  %t61 = zext i1 %t60 to i32
  %t62 = icmp ne i32 %t61, 0
  br i1 %t62, label %block_5, label %block_6
block_5:
  %t63 = call i32 @abort()
  br label %block_6
block_6:
  %t64 = getelementptr %struct.tiny, ptr %lv.x, i32 0, i32 2
  %t65 = load i8, ptr %t64
  %t66 = load i32, ptr %lv.i
  %t67 = add i32 %t66, 30
  %t68 = sext i8 %t65 to i32
  %t69 = icmp ne i32 %t68, %t67
  %t70 = zext i1 %t69 to i32
  %t71 = icmp ne i32 %t70, 0
  br i1 %t71, label %block_7, label %block_8
block_7:
  %t72 = call i32 @abort()
  br label %block_8
block_8:
  %t73 = getelementptr %struct.tiny, ptr %lv.x, i32 0, i32 3
  %t74 = load i8, ptr %t73
  %t75 = load i32, ptr %lv.i
  %t76 = add i32 %t75, 40
  %t77 = sext i8 %t74 to i32
  %t78 = icmp ne i32 %t77, %t76
  %t79 = zext i1 %t78 to i32
  %t80 = icmp ne i32 %t79, 0
  br i1 %t80, label %block_9, label %block_10
block_9:
  %t81 = call i32 @abort()
  br label %block_10
block_10:
  %t82 = getelementptr %struct.tiny, ptr %lv.x, i32 0, i32 4
  %t83 = load i8, ptr %t82
  %t84 = load i32, ptr %lv.i
  %t85 = add i32 %t84, 50
  %t86 = sext i8 %t83 to i32
  %t87 = icmp ne i32 %t86, %t85
  %t88 = zext i1 %t87 to i32
  %t89 = icmp ne i32 %t88, 0
  br i1 %t89, label %block_11, label %block_12
block_11:
  %t90 = call i32 @abort()
  br label %block_12
block_12:
  br label %for.latch.1
block_13:
  %t91 = call i32 @abort()
  br label %block_14
block_14:
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret i32 0
}

define i32 @main()
{
entry:
  %lv.x = alloca [3 x %struct.tiny], align 1
  %t0 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t1 = sext i32 0 to i64
  %t2 = getelementptr %struct.tiny, ptr %t0, i64 %t1
  %t3 = getelementptr %struct.tiny, ptr %t2, i32 0, i32 0
  %t4 = trunc i32 10 to i8
  store i8 %t4, ptr %t3
  %t5 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t6 = sext i32 1 to i64
  %t7 = getelementptr %struct.tiny, ptr %t5, i64 %t6
  %t8 = getelementptr %struct.tiny, ptr %t7, i32 0, i32 0
  %t9 = trunc i32 11 to i8
  store i8 %t9, ptr %t8
  %t10 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t11 = sext i32 2 to i64
  %t12 = getelementptr %struct.tiny, ptr %t10, i64 %t11
  %t13 = getelementptr %struct.tiny, ptr %t12, i32 0, i32 0
  %t14 = trunc i32 12 to i8
  store i8 %t14, ptr %t13
  %t15 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t16 = sext i32 0 to i64
  %t17 = getelementptr %struct.tiny, ptr %t15, i64 %t16
  %t18 = getelementptr %struct.tiny, ptr %t17, i32 0, i32 1
  %t19 = trunc i32 20 to i8
  store i8 %t19, ptr %t18
  %t20 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t21 = sext i32 1 to i64
  %t22 = getelementptr %struct.tiny, ptr %t20, i64 %t21
  %t23 = getelementptr %struct.tiny, ptr %t22, i32 0, i32 1
  %t24 = trunc i32 21 to i8
  store i8 %t24, ptr %t23
  %t25 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t26 = sext i32 2 to i64
  %t27 = getelementptr %struct.tiny, ptr %t25, i64 %t26
  %t28 = getelementptr %struct.tiny, ptr %t27, i32 0, i32 1
  %t29 = trunc i32 22 to i8
  store i8 %t29, ptr %t28
  %t30 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t31 = sext i32 0 to i64
  %t32 = getelementptr %struct.tiny, ptr %t30, i64 %t31
  %t33 = getelementptr %struct.tiny, ptr %t32, i32 0, i32 2
  %t34 = trunc i32 30 to i8
  store i8 %t34, ptr %t33
  %t35 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t36 = sext i32 1 to i64
  %t37 = getelementptr %struct.tiny, ptr %t35, i64 %t36
  %t38 = getelementptr %struct.tiny, ptr %t37, i32 0, i32 2
  %t39 = trunc i32 31 to i8
  store i8 %t39, ptr %t38
  %t40 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t41 = sext i32 2 to i64
  %t42 = getelementptr %struct.tiny, ptr %t40, i64 %t41
  %t43 = getelementptr %struct.tiny, ptr %t42, i32 0, i32 2
  %t44 = trunc i32 32 to i8
  store i8 %t44, ptr %t43
  %t45 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t46 = sext i32 0 to i64
  %t47 = getelementptr %struct.tiny, ptr %t45, i64 %t46
  %t48 = getelementptr %struct.tiny, ptr %t47, i32 0, i32 3
  %t49 = trunc i32 40 to i8
  store i8 %t49, ptr %t48
  %t50 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t51 = sext i32 1 to i64
  %t52 = getelementptr %struct.tiny, ptr %t50, i64 %t51
  %t53 = getelementptr %struct.tiny, ptr %t52, i32 0, i32 3
  %t54 = trunc i32 41 to i8
  store i8 %t54, ptr %t53
  %t55 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t56 = sext i32 2 to i64
  %t57 = getelementptr %struct.tiny, ptr %t55, i64 %t56
  %t58 = getelementptr %struct.tiny, ptr %t57, i32 0, i32 3
  %t59 = trunc i32 42 to i8
  store i8 %t59, ptr %t58
  %t60 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t61 = sext i32 0 to i64
  %t62 = getelementptr %struct.tiny, ptr %t60, i64 %t61
  %t63 = getelementptr %struct.tiny, ptr %t62, i32 0, i32 4
  %t64 = trunc i32 50 to i8
  store i8 %t64, ptr %t63
  %t65 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t66 = sext i32 1 to i64
  %t67 = getelementptr %struct.tiny, ptr %t65, i64 %t66
  %t68 = getelementptr %struct.tiny, ptr %t67, i32 0, i32 4
  %t69 = trunc i32 51 to i8
  store i8 %t69, ptr %t68
  %t70 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t71 = sext i32 2 to i64
  %t72 = getelementptr %struct.tiny, ptr %t70, i64 %t71
  %t73 = getelementptr %struct.tiny, ptr %t72, i32 0, i32 4
  %t74 = trunc i32 52 to i8
  store i8 %t74, ptr %t73
  %t75 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t76 = sext i32 0 to i64
  %t77 = getelementptr %struct.tiny, ptr %t75, i64 %t76
  %t78 = load %struct.tiny, ptr %t77
  %t79 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t80 = sext i32 0 to i64
  %t81 = getelementptr %struct.tiny, ptr %t79, i64 %t80
  %t82 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t82, ptr %t81, i64 5, i1 false)
  %t83 = load i64, ptr %t82
  %t84 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t85 = sext i32 1 to i64
  %t86 = getelementptr %struct.tiny, ptr %t84, i64 %t85
  %t87 = load %struct.tiny, ptr %t86
  %t88 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t89 = sext i32 1 to i64
  %t90 = getelementptr %struct.tiny, ptr %t88, i64 %t89
  %t91 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t91, ptr %t90, i64 5, i1 false)
  %t92 = load i64, ptr %t91
  %t93 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t94 = sext i32 2 to i64
  %t95 = getelementptr %struct.tiny, ptr %t93, i64 %t94
  %t96 = load %struct.tiny, ptr %t95
  %t97 = getelementptr [3 x %struct.tiny], ptr %lv.x, i64 0, i64 0
  %t98 = sext i32 2 to i64
  %t99 = getelementptr %struct.tiny, ptr %t97, i64 %t98
  %t100 = alloca i64
  call void @llvm.memcpy.p0.p0.i64(ptr %t100, ptr %t99, i64 5, i1 false)
  %t101 = load i64, ptr %t100
  %t102 = sext i32 123 to i64
  %t103 = call i32 (i32, ...) @f(i32 3, i64 %t83, i64 %t92, i64 %t101, i64 %t102)
  %t104 = call i32 @exit(i32 0)
  ret i32 0
}

