target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }
@foo_arg = global i32 zeroinitializer, align 4
@bar_arg = global i32 zeroinitializer, align 4
@x = global i64 zeroinitializer, align 8
@d = global double zeroinitializer, align 8
@gap = global %struct.__va_list_tag_ zeroinitializer, align 8
@pap = global ptr null, align 8

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)
declare void @llvm.va_copy.p0.p0(ptr, ptr)
declare void @llvm.memcpy.p0.p0.i64(ptr, ptr, i64, i1)

declare void @abort()

define void @foo(i32 %p.v, ptr %p.ap)
{
entry:
  switch i32 %p.v, label %block_4 [
    i32 5, label %block_3
  ]
block_1:
  br label %block_3
block_2:
  ret void
block_3:
  %t0 = getelementptr %struct.__va_list_tag_, ptr %p.ap, i32 0, i32 3
  %t1 = load i32, ptr %t0
  %t6 = icmp sge i32 %t1, 0
  br i1 %t6, label %vaarg.stack.2, label %vaarg.regtry.3
vaarg.regtry.3:
  %t7 = add i32 %t1, 8
  store i32 %t7, ptr %t0
  %t8 = icmp sle i32 %t7, 0
  br i1 %t8, label %vaarg.reg.4, label %vaarg.stack.2
vaarg.reg.4:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %p.ap, i32 0, i32 1
  %t10 = load ptr, ptr %t9
  %t11 = getelementptr i8, ptr %t10, i32 %t1
  br label %vaarg.join.5
vaarg.stack.2:
  %t12 = getelementptr %struct.__va_list_tag_, ptr %p.ap, i32 0, i32 0
  %t13 = load ptr, ptr %t12
  %t14 = getelementptr i8, ptr %t13, i64 8
  store ptr %t14, ptr %t12
  br label %vaarg.join.5
vaarg.join.5:
  %t15 = phi ptr [ %t11, %vaarg.reg.4 ], [ %t13, %vaarg.stack.2 ]
  %t16 = load i32, ptr %t15
  store i32 %t16, ptr @foo_arg
  br label %block_2
block_4:
  call void () @abort()
  br label %block_2
}

define void @bar(i32 %p.v)
{
entry:
  %t0 = icmp eq i32 %p.v, 16390
  %t1 = zext i1 %t0 to i32
  %t2 = icmp ne i32 %t1, 0
  br i1 %t2, label %block_6, label %block_7
block_6:
  %t3 = getelementptr %struct.__va_list_tag_, ptr @gap, i32 0, i32 4
  %t4 = load i32, ptr %t3
  %t9 = icmp sge i32 %t4, 0
  br i1 %t9, label %vaarg.fp.stack.5, label %vaarg.fp.regtry.6
vaarg.fp.regtry.6:
  %t10 = add i32 %t4, 16
  store i32 %t10, ptr %t3
  %t11 = icmp sle i32 %t10, 0
  br i1 %t11, label %vaarg.fp.reg.7, label %vaarg.fp.stack.5
vaarg.fp.reg.7:
  %t12 = getelementptr %struct.__va_list_tag_, ptr @gap, i32 0, i32 2
  %t13 = load ptr, ptr %t12
  %t14 = getelementptr i8, ptr %t13, i32 %t4
  br label %vaarg.fp.join.8
vaarg.fp.stack.5:
  %t15 = getelementptr %struct.__va_list_tag_, ptr @gap, i32 0, i32 0
  %t16 = load ptr, ptr %t15
  %t17 = ptrtoint ptr %t16 to i64
  %t18 = add i64 %t17, 7
  %t19 = and i64 %t18, -8
  %t20 = inttoptr i64 %t19 to ptr
  %t21 = getelementptr i8, ptr %t20, i64 8
  store ptr %t21, ptr %t15
  br label %vaarg.fp.join.8
vaarg.fp.join.8:
  %t22 = phi ptr [ %t14, %vaarg.fp.reg.7 ], [ %t20, %vaarg.fp.stack.5 ]
  %t23 = load double, ptr %t22
  %t24 = fcmp une double %t23, 0x4031000000000000
  %t25 = zext i1 %t24 to i32
  %t26 = icmp ne i32 %t25, 0
  br i1 %t26, label %logic.skip.28, label %logic.rhs.27
logic.rhs.27:
  %t31 = getelementptr %struct.__va_list_tag_, ptr @gap, i32 0, i32 3
  %t32 = load i32, ptr %t31
  %t37 = icmp sge i32 %t32, 0
  br i1 %t37, label %vaarg.stack.33, label %vaarg.regtry.34
vaarg.regtry.34:
  %t38 = add i32 %t32, 8
  store i32 %t38, ptr %t31
  %t39 = icmp sle i32 %t38, 0
  br i1 %t39, label %vaarg.reg.35, label %vaarg.stack.33
vaarg.reg.35:
  %t40 = getelementptr %struct.__va_list_tag_, ptr @gap, i32 0, i32 1
  %t41 = load ptr, ptr %t40
  %t42 = getelementptr i8, ptr %t41, i32 %t32
  br label %vaarg.join.36
vaarg.stack.33:
  %t43 = getelementptr %struct.__va_list_tag_, ptr @gap, i32 0, i32 0
  %t44 = load ptr, ptr %t43
  %t45 = getelementptr i8, ptr %t44, i64 8
  store ptr %t45, ptr %t43
  br label %vaarg.join.36
vaarg.join.36:
  %t46 = phi ptr [ %t42, %vaarg.reg.35 ], [ %t44, %vaarg.stack.33 ]
  %t47 = load i64, ptr %t46
  %t48 = icmp ne i64 %t47, 129
  %t49 = zext i1 %t48 to i32
  %t50 = icmp ne i32 %t49, 0
  %t51 = zext i1 %t50 to i32
  br label %logic.rhs.end.29
logic.rhs.end.29:
  br label %logic.end.30
logic.skip.28:
  br label %logic.end.30
logic.end.30:
  %t52 = phi i32 [ %t51, %logic.rhs.end.29 ], [ 1, %logic.skip.28 ]
  %t53 = icmp ne i32 %t52, 0
  br i1 %t53, label %block_9, label %block_10
block_7:
  %t54 = icmp eq i32 %p.v, 16392
  %t55 = zext i1 %t54 to i32
  %t56 = icmp ne i32 %t55, 0
  br i1 %t56, label %block_11, label %block_12
block_8:
  store i32 %p.v, ptr @bar_arg
  ret void
block_9:
  call void () @abort()
  br label %block_10
block_10:
  br label %block_8
block_11:
  %t57 = load ptr, ptr @pap
  %t58 = getelementptr %struct.__va_list_tag_, ptr %t57, i32 0, i32 3
  %t59 = load i32, ptr %t58
  %t64 = icmp sge i32 %t59, 0
  br i1 %t64, label %vaarg.stack.60, label %vaarg.regtry.61
vaarg.regtry.61:
  %t65 = add i32 %t59, 8
  store i32 %t65, ptr %t58
  %t66 = icmp sle i32 %t65, 0
  br i1 %t66, label %vaarg.reg.62, label %vaarg.stack.60
vaarg.reg.62:
  %t67 = getelementptr %struct.__va_list_tag_, ptr %t57, i32 0, i32 1
  %t68 = load ptr, ptr %t67
  %t69 = getelementptr i8, ptr %t68, i32 %t59
  br label %vaarg.join.63
vaarg.stack.60:
  %t70 = getelementptr %struct.__va_list_tag_, ptr %t57, i32 0, i32 0
  %t71 = load ptr, ptr %t70
  %t72 = getelementptr i8, ptr %t71, i64 8
  store ptr %t72, ptr %t70
  br label %vaarg.join.63
vaarg.join.63:
  %t73 = phi ptr [ %t69, %vaarg.reg.62 ], [ %t71, %vaarg.stack.60 ]
  %t74 = load i64, ptr %t73
  %t75 = icmp ne i64 %t74, 14
  %t76 = zext i1 %t75 to i32
  %t77 = icmp ne i32 %t76, 0
  br i1 %t77, label %logic.skip.79, label %logic.rhs.78
logic.rhs.78:
  %t82 = load ptr, ptr @pap
  %t83 = getelementptr %struct.__va_list_tag_, ptr %t82, i32 0, i32 4
  %t84 = load i32, ptr %t83
  %t89 = icmp sge i32 %t84, 0
  br i1 %t89, label %vaarg.fp.stack.85, label %vaarg.fp.regtry.86
vaarg.fp.regtry.86:
  %t90 = add i32 %t84, 16
  store i32 %t90, ptr %t83
  %t91 = icmp sle i32 %t90, 0
  br i1 %t91, label %vaarg.fp.reg.87, label %vaarg.fp.stack.85
vaarg.fp.reg.87:
  %t92 = getelementptr %struct.__va_list_tag_, ptr %t82, i32 0, i32 2
  %t93 = load ptr, ptr %t92
  %t94 = getelementptr i8, ptr %t93, i32 %t84
  br label %vaarg.fp.join.88
vaarg.fp.stack.85:
  %t95 = getelementptr %struct.__va_list_tag_, ptr %t82, i32 0, i32 0
  %t96 = load ptr, ptr %t95
  %t97 = ptrtoint ptr %t96 to i64
  %t98 = add i64 %t97, 15
  %t99 = and i64 %t98, -16
  %t100 = inttoptr i64 %t99 to ptr
  %t101 = getelementptr i8, ptr %t100, i64 16
  store ptr %t101, ptr %t95
  br label %vaarg.fp.join.88
vaarg.fp.join.88:
  %t102 = phi ptr [ %t94, %vaarg.fp.reg.87 ], [ %t100, %vaarg.fp.stack.85 ]
  %t103 = load fp128, ptr %t102
  %t104 = fcmp une fp128 %t103, 0xL00000000000000004006060000000000
  %t105 = zext i1 %t104 to i32
  %t106 = icmp ne i32 %t105, 0
  %t107 = zext i1 %t106 to i32
  br label %logic.rhs.end.80
logic.rhs.end.80:
  br label %logic.end.81
logic.skip.79:
  br label %logic.end.81
logic.end.81:
  %t108 = phi i32 [ %t107, %logic.rhs.end.80 ], [ 1, %logic.skip.79 ]
  %t109 = icmp ne i32 %t108, 0
  br i1 %t109, label %logic.skip.111, label %logic.rhs.110
logic.rhs.110:
  %t114 = load ptr, ptr @pap
  %t115 = getelementptr %struct.__va_list_tag_, ptr %t114, i32 0, i32 3
  %t116 = load i32, ptr %t115
  %t121 = icmp sge i32 %t116, 0
  br i1 %t121, label %vaarg.stack.117, label %vaarg.regtry.118
vaarg.regtry.118:
  %t122 = add i32 %t116, 8
  store i32 %t122, ptr %t115
  %t123 = icmp sle i32 %t122, 0
  br i1 %t123, label %vaarg.reg.119, label %vaarg.stack.117
vaarg.reg.119:
  %t124 = getelementptr %struct.__va_list_tag_, ptr %t114, i32 0, i32 1
  %t125 = load ptr, ptr %t124
  %t126 = getelementptr i8, ptr %t125, i32 %t116
  br label %vaarg.join.120
vaarg.stack.117:
  %t127 = getelementptr %struct.__va_list_tag_, ptr %t114, i32 0, i32 0
  %t128 = load ptr, ptr %t127
  %t129 = getelementptr i8, ptr %t128, i64 8
  store ptr %t129, ptr %t127
  br label %vaarg.join.120
vaarg.join.120:
  %t130 = phi ptr [ %t126, %vaarg.reg.119 ], [ %t128, %vaarg.stack.117 ]
  %t131 = load i32, ptr %t130
  %t132 = icmp ne i32 %t131, 17
  %t133 = zext i1 %t132 to i32
  %t134 = icmp ne i32 %t133, 0
  %t135 = zext i1 %t134 to i32
  br label %logic.rhs.end.112
logic.rhs.end.112:
  br label %logic.end.113
logic.skip.111:
  br label %logic.end.113
logic.end.113:
  %t136 = phi i32 [ %t135, %logic.rhs.end.112 ], [ 1, %logic.skip.111 ]
  %t137 = icmp ne i32 %t136, 0
  br i1 %t137, label %block_13, label %block_14
block_12:
  br label %block_8
block_13:
  call void () @abort()
  br label %block_14
block_14:
  br label %block_12
}

define void @f0(i32 %p.i, ...)
{
entry:
  ret void
}

define void @f1(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define void @f2(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  %t0 = load double, ptr @d
  %t1 = fptosi double %t0 to i32
  call void (i32) @bar(i32 %t1)
  %t2 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t3 = load i32, ptr %t2
  %t8 = icmp sge i32 %t3, 0
  br i1 %t8, label %vaarg.stack.4, label %vaarg.regtry.5
vaarg.regtry.5:
  %t9 = add i32 %t3, 8
  store i32 %t9, ptr %t2
  %t10 = icmp sle i32 %t9, 0
  br i1 %t10, label %vaarg.reg.6, label %vaarg.stack.4
vaarg.reg.6:
  %t11 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t12 = load ptr, ptr %t11
  %t13 = getelementptr i8, ptr %t12, i32 %t3
  br label %vaarg.join.7
vaarg.stack.4:
  %t14 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t15 = load ptr, ptr %t14
  %t16 = getelementptr i8, ptr %t15, i64 8
  store ptr %t16, ptr %t14
  br label %vaarg.join.7
vaarg.join.7:
  %t17 = phi ptr [ %t13, %vaarg.reg.6 ], [ %t15, %vaarg.stack.4 ]
  %t18 = load i64, ptr %t17
  store i64 %t18, ptr @x
  %t19 = load i64, ptr @x
  %t20 = trunc i64 %t19 to i32
  call void (i32) @bar(i32 %t20)
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define void @f3(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  %t0 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4
  %t1 = load i32, ptr %t0
  %t6 = icmp sge i32 %t1, 0
  br i1 %t6, label %vaarg.fp.stack.2, label %vaarg.fp.regtry.3
vaarg.fp.regtry.3:
  %t7 = add i32 %t1, 16
  store i32 %t7, ptr %t0
  %t8 = icmp sle i32 %t7, 0
  br i1 %t8, label %vaarg.fp.reg.4, label %vaarg.fp.stack.2
vaarg.fp.reg.4:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 2
  %t10 = load ptr, ptr %t9
  %t11 = getelementptr i8, ptr %t10, i32 %t1
  br label %vaarg.fp.join.5
vaarg.fp.stack.2:
  %t12 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t13 = load ptr, ptr %t12
  %t14 = ptrtoint ptr %t13 to i64
  %t15 = add i64 %t14, 7
  %t16 = and i64 %t15, -8
  %t17 = inttoptr i64 %t16 to ptr
  %t18 = getelementptr i8, ptr %t17, i64 8
  store ptr %t18, ptr %t12
  br label %vaarg.fp.join.5
vaarg.fp.join.5:
  %t19 = phi ptr [ %t11, %vaarg.fp.reg.4 ], [ %t17, %vaarg.fp.stack.2 ]
  %t20 = load double, ptr %t19
  store double %t20, ptr @d
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define void @f4(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  %t0 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4
  %t1 = load i32, ptr %t0
  %t6 = icmp sge i32 %t1, 0
  br i1 %t6, label %vaarg.fp.stack.2, label %vaarg.fp.regtry.3
vaarg.fp.regtry.3:
  %t7 = add i32 %t1, 16
  store i32 %t7, ptr %t0
  %t8 = icmp sle i32 %t7, 0
  br i1 %t8, label %vaarg.fp.reg.4, label %vaarg.fp.stack.2
vaarg.fp.reg.4:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 2
  %t10 = load ptr, ptr %t9
  %t11 = getelementptr i8, ptr %t10, i32 %t1
  br label %vaarg.fp.join.5
vaarg.fp.stack.2:
  %t12 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t13 = load ptr, ptr %t12
  %t14 = ptrtoint ptr %t13 to i64
  %t15 = add i64 %t14, 7
  %t16 = and i64 %t15, -8
  %t17 = inttoptr i64 %t16 to ptr
  %t18 = getelementptr i8, ptr %t17, i64 8
  store ptr %t18, ptr %t12
  br label %vaarg.fp.join.5
vaarg.fp.join.5:
  %t19 = phi ptr [ %t11, %vaarg.fp.reg.4 ], [ %t17, %vaarg.fp.stack.2 ]
  %t20 = load double, ptr %t19
  %t21 = fptosi double %t20 to i64
  store i64 %t21, ptr @x
  %t22 = alloca %struct.__va_list_tag_
  call void @llvm.memcpy.p0.p0.i64(ptr %t22, ptr %lv.ap, i64 32, i1 false)
  call void (i32, ptr) @foo(i32 %p.i, ptr %t22)
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define void @f5(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  call void @llvm.va_copy.p0.p0(ptr @gap, ptr %lv.ap)
  call void (i32) @bar(i32 %p.i)
  call void @llvm.va_end.p0(ptr %lv.ap)
  call void @llvm.va_end.p0(ptr @gap)
  ret void
}

define void @f6(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  %t0 = load double, ptr @d
  %t1 = fptosi double %t0 to i32
  call void (i32) @bar(i32 %t1)
  %t2 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t3 = load i32, ptr %t2
  %t8 = icmp sge i32 %t3, 0
  br i1 %t8, label %vaarg.stack.4, label %vaarg.regtry.5
vaarg.regtry.5:
  %t9 = add i32 %t3, 8
  store i32 %t9, ptr %t2
  %t10 = icmp sle i32 %t9, 0
  br i1 %t10, label %vaarg.reg.6, label %vaarg.stack.4
vaarg.reg.6:
  %t11 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t12 = load ptr, ptr %t11
  %t13 = getelementptr i8, ptr %t12, i32 %t3
  br label %vaarg.join.7
vaarg.stack.4:
  %t14 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t15 = load ptr, ptr %t14
  %t16 = getelementptr i8, ptr %t15, i64 8
  store ptr %t16, ptr %t14
  br label %vaarg.join.7
vaarg.join.7:
  %t17 = phi ptr [ %t13, %vaarg.reg.6 ], [ %t15, %vaarg.stack.4 ]
  %t18 = load i64, ptr %t17
  %t19 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t20 = load i32, ptr %t19
  %t25 = icmp sge i32 %t20, 0
  br i1 %t25, label %vaarg.stack.21, label %vaarg.regtry.22
vaarg.regtry.22:
  %t26 = add i32 %t20, 8
  store i32 %t26, ptr %t19
  %t27 = icmp sle i32 %t26, 0
  br i1 %t27, label %vaarg.reg.23, label %vaarg.stack.21
vaarg.reg.23:
  %t28 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t29 = load ptr, ptr %t28
  %t30 = getelementptr i8, ptr %t29, i32 %t20
  br label %vaarg.join.24
vaarg.stack.21:
  %t31 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t32 = load ptr, ptr %t31
  %t33 = getelementptr i8, ptr %t32, i64 8
  store ptr %t33, ptr %t31
  br label %vaarg.join.24
vaarg.join.24:
  %t34 = phi ptr [ %t30, %vaarg.reg.23 ], [ %t32, %vaarg.stack.21 ]
  %t35 = load i64, ptr %t34
  %t36 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 3
  %t37 = load i32, ptr %t36
  %t42 = icmp sge i32 %t37, 0
  br i1 %t42, label %vaarg.stack.38, label %vaarg.regtry.39
vaarg.regtry.39:
  %t43 = add i32 %t37, 8
  store i32 %t43, ptr %t36
  %t44 = icmp sle i32 %t43, 0
  br i1 %t44, label %vaarg.reg.40, label %vaarg.stack.38
vaarg.reg.40:
  %t45 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 1
  %t46 = load ptr, ptr %t45
  %t47 = getelementptr i8, ptr %t46, i32 %t37
  br label %vaarg.join.41
vaarg.stack.38:
  %t48 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t49 = load ptr, ptr %t48
  %t50 = getelementptr i8, ptr %t49, i64 8
  store ptr %t50, ptr %t48
  br label %vaarg.join.41
vaarg.join.41:
  %t51 = phi ptr [ %t47, %vaarg.reg.40 ], [ %t49, %vaarg.stack.38 ]
  %t52 = load i64, ptr %t51
  store i64 %t52, ptr @x
  %t53 = load i64, ptr @x
  %t54 = trunc i64 %t53 to i32
  call void (i32) @bar(i32 %t54)
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define void @f7(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  store ptr %lv.ap, ptr @pap
  call void (i32) @bar(i32 %p.i)
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define void @f8(i32 %p.i, ...)
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  call void @llvm.va_start.p0(ptr %lv.ap)
  store ptr %lv.ap, ptr @pap
  call void (i32) @bar(i32 %p.i)
  %t0 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 4
  %t1 = load i32, ptr %t0
  %t6 = icmp sge i32 %t1, 0
  br i1 %t6, label %vaarg.fp.stack.2, label %vaarg.fp.regtry.3
vaarg.fp.regtry.3:
  %t7 = add i32 %t1, 16
  store i32 %t7, ptr %t0
  %t8 = icmp sle i32 %t7, 0
  br i1 %t8, label %vaarg.fp.reg.4, label %vaarg.fp.stack.2
vaarg.fp.reg.4:
  %t9 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 2
  %t10 = load ptr, ptr %t9
  %t11 = getelementptr i8, ptr %t10, i32 %t1
  br label %vaarg.fp.join.5
vaarg.fp.stack.2:
  %t12 = getelementptr %struct.__va_list_tag_, ptr %lv.ap, i32 0, i32 0
  %t13 = load ptr, ptr %t12
  %t14 = ptrtoint ptr %t13 to i64
  %t15 = add i64 %t14, 7
  %t16 = and i64 %t15, -8
  %t17 = inttoptr i64 %t16 to ptr
  %t18 = getelementptr i8, ptr %t17, i64 8
  store ptr %t18, ptr %t12
  br label %vaarg.fp.join.5
vaarg.fp.join.5:
  %t19 = phi ptr [ %t11, %vaarg.fp.reg.4 ], [ %t17, %vaarg.fp.stack.2 ]
  %t20 = load double, ptr %t19
  store double %t20, ptr @d
  call void @llvm.va_end.p0(ptr %lv.ap)
  ret void
}

define i32 @main()
{
entry:
  call void (i32, ...) @f0(i32 1)
  call void (i32, ...) @f1(i32 2)
  store double 0x403F000000000000, ptr @d
  call void (i32, ...) @f2(i32 3, i64 28)
  %t0 = load i32, ptr @bar_arg
  %t1 = icmp ne i32 %t0, 28
  %t2 = zext i1 %t1 to i32
  %t3 = icmp ne i32 %t2, 0
  br i1 %t3, label %logic.skip.5, label %logic.rhs.4
logic.rhs.4:
  %t8 = load i64, ptr @x
  %t9 = sext i32 28 to i64
  %t10 = icmp ne i64 %t8, %t9
  %t11 = zext i1 %t10 to i32
  %t12 = icmp ne i32 %t11, 0
  %t13 = zext i1 %t12 to i32
  br label %logic.rhs.end.6
logic.rhs.end.6:
  br label %logic.end.7
logic.skip.5:
  br label %logic.end.7
logic.end.7:
  %t14 = phi i32 [ %t13, %logic.rhs.end.6 ], [ 1, %logic.skip.5 ]
  %t15 = icmp ne i32 %t14, 0
  br i1 %t15, label %block_25, label %block_26
block_25:
  call void () @abort()
  br label %block_26
block_26:
  call void (i32, ...) @f3(i32 4, double 0x4060600000000000)
  %t16 = load double, ptr @d
  %t17 = fcmp une double %t16, 0x4060600000000000
  %t18 = zext i1 %t17 to i32
  %t19 = icmp ne i32 %t18, 0
  br i1 %t19, label %block_27, label %block_28
block_27:
  call void () @abort()
  br label %block_28
block_28:
  call void (i32, ...) @f4(i32 5, double 0x4030000000000000, i32 128)
  %t20 = load i64, ptr @x
  %t21 = sext i32 16 to i64
  %t22 = icmp ne i64 %t20, %t21
  %t23 = zext i1 %t22 to i32
  %t24 = icmp ne i32 %t23, 0
  br i1 %t24, label %logic.skip.26, label %logic.rhs.25
logic.rhs.25:
  %t29 = load i32, ptr @foo_arg
  %t30 = icmp ne i32 %t29, 128
  %t31 = zext i1 %t30 to i32
  %t32 = icmp ne i32 %t31, 0
  %t33 = zext i1 %t32 to i32
  br label %logic.rhs.end.27
logic.rhs.end.27:
  br label %logic.end.28
logic.skip.26:
  br label %logic.end.28
logic.end.28:
  %t34 = phi i32 [ %t33, %logic.rhs.end.27 ], [ 1, %logic.skip.26 ]
  %t35 = icmp ne i32 %t34, 0
  br i1 %t35, label %block_29, label %block_30
block_29:
  call void () @abort()
  br label %block_30
block_30:
  call void (i32, ...) @f5(i32 16390, double 0x4031000000000000, i64 129)
  %t36 = load i32, ptr @bar_arg
  %t37 = icmp ne i32 %t36, 16390
  %t38 = zext i1 %t37 to i32
  %t39 = icmp ne i32 %t38, 0
  br i1 %t39, label %block_31, label %block_32
block_31:
  call void () @abort()
  br label %block_32
block_32:
  %t40 = sub i64 0, 31
  call void (i32, ...) @f6(i32 7, i64 12, i64 14, i64 %t40)
  %t41 = load i32, ptr @bar_arg
  %t42 = sub i32 0, 31
  %t43 = icmp ne i32 %t41, %t42
  %t44 = zext i1 %t43 to i32
  %t45 = icmp ne i32 %t44, 0
  br i1 %t45, label %block_33, label %block_34
block_33:
  call void () @abort()
  br label %block_34
block_34:
  call void (i32, ...) @f7(i32 16392, i64 14, fp128 0xL00000000000000004006060000000000, i32 17, double 0x403A000000000000)
  %t46 = load i32, ptr @bar_arg
  %t47 = icmp ne i32 %t46, 16392
  %t48 = zext i1 %t47 to i32
  %t49 = icmp ne i32 %t48, 0
  br i1 %t49, label %block_35, label %block_36
block_35:
  call void () @abort()
  br label %block_36
block_36:
  call void (i32, ...) @f8(i32 16392, i64 14, fp128 0xL00000000000000004006060000000000, i32 17, double 0x403B000000000000)
  %t50 = load i32, ptr @bar_arg
  %t51 = icmp ne i32 %t50, 16392
  %t52 = zext i1 %t51 to i32
  %t53 = icmp ne i32 %t52, 0
  br i1 %t53, label %logic.skip.55, label %logic.rhs.54
logic.rhs.54:
  %t58 = load double, ptr @d
  %t59 = fcmp une double %t58, 0x403B000000000000
  %t60 = zext i1 %t59 to i32
  %t61 = icmp ne i32 %t60, 0
  %t62 = zext i1 %t61 to i32
  br label %logic.rhs.end.56
logic.rhs.end.56:
  br label %logic.end.57
logic.skip.55:
  br label %logic.end.57
logic.end.57:
  %t63 = phi i32 [ %t62, %logic.rhs.end.56 ], [ 1, %logic.skip.55 ]
  %t64 = icmp ne i32 %t63, 0
  br i1 %t64, label %block_37, label %block_38
block_37:
  call void () @abort()
  br label %block_38
block_38:
  ret i32 0
}

