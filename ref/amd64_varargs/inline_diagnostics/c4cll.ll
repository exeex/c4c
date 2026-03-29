target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }

declare void @llvm.va_start.p0(ptr)
declare void @llvm.va_end.p0(ptr)

define i32 @noinline_add(i32 %p.x, i32 %p.y) noinline
{
entry:
  %t0 = add i32 %p.x, %p.y
  ret i32 %t0
}

define i32 @default_mul(i32 %p.x, i32 %p.y)
{
entry:
  %t0 = mul i32 %p.x, %p.y
  ret i32 %t0
}

define internal i32 @recursive_fib(i32 %p.n) alwaysinline
{
entry:
  %lv.inl_hoist_recursive_fib = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.1 = alloca i32, align 4
  %t0 = icmp sle i32 %p.n, 1
  %t1 = zext i1 %t0 to i32
  %t2 = icmp ne i32 %t1, 0
  br i1 %t2, label %block_3, label %block_4
block_3:
  ret i32 %p.n
block_4:
  %t3 = sub i32 %p.n, 1
  %t4 = call i32 (i32) @recursive_fib(i32 %t3)
  store i32 %t4, ptr %lv.inl_hoist_recursive_fib
  %t5 = sub i32 %p.n, 2
  %t6 = call i32 (i32) @recursive_fib(i32 %t5)
  store i32 %t6, ptr %lv.inl_hoist_recursive_fib.1
  %t7 = load i32, ptr %lv.inl_hoist_recursive_fib
  %t8 = load i32, ptr %lv.inl_hoist_recursive_fib.1
  %t9 = add i32 %t7, %t8
  ret i32 %t9
}

define internal i32 @variadic_sum(i32 %p.count, ...) alwaysinline
{
entry:
  %lv.ap = alloca %struct.__va_list_tag_, align 8
  %lv.sum = alloca i32, align 4
  %lv.i = alloca i32, align 4
  call void @llvm.va_start.p0(ptr %lv.ap)
  store i32 0, ptr %lv.sum
  store i32 0, ptr %lv.i
  br label %for.cond.6
for.cond.6:
  %t0 = load i32, ptr %lv.i
  %t1 = icmp slt i32 %t0, %p.count
  %t2 = zext i1 %t1 to i32
  %t3 = icmp ne i32 %t2, 0
  br i1 %t3, label %block_6, label %block_7
for.latch.6:
  %t4 = load i32, ptr %lv.i
  %t5 = add i32 %t4, 1
  store i32 %t5, ptr %lv.i
  br label %for.cond.6
block_6:
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
  %t22 = load i32, ptr %t21
  %t23 = load i32, ptr %lv.sum
  %t24 = add i32 %t23, %t22
  store i32 %t24, ptr %lv.sum
  br label %for.latch.6
block_7:
  call void @llvm.va_end.p0(ptr %lv.ap)
  %t25 = load i32, ptr %lv.sum
  ret i32 %t25
}

define i32 @noinline_wins(i32 %p.x) noinline
{
entry:
  %t0 = add i32 %p.x, 100
  ret i32 %t0
}

define i32 @main()
{
entry:
  %lv.inl_recursive_fib_result = alloca i32, align 4
  %lv.inl_recursive_fib_n = alloca i32, align 4
  %lv.inl_recursive_fib_result.1 = alloca i32, align 4
  %lv.inl_recursive_fib_n.1 = alloca i32, align 4
  %lv.inl_recursive_fib_result.2 = alloca i32, align 4
  %lv.inl_recursive_fib_n.2 = alloca i32, align 4
  %lv.inl_recursive_fib_result.3 = alloca i32, align 4
  %lv.inl_recursive_fib_n.3 = alloca i32, align 4
  %lv.inl_recursive_fib_result.4 = alloca i32, align 4
  %lv.inl_recursive_fib_n.4 = alloca i32, align 4
  %lv.inl_recursive_fib_result.5 = alloca i32, align 4
  %lv.inl_recursive_fib_n.5 = alloca i32, align 4
  %lv.inl_recursive_fib_result.6 = alloca i32, align 4
  %lv.inl_recursive_fib_n.6 = alloca i32, align 4
  %lv.inl_recursive_fib_result.7 = alloca i32, align 4
  %lv.inl_recursive_fib_n.7 = alloca i32, align 4
  %lv.inl_recursive_fib_result.8 = alloca i32, align 4
  %lv.inl_recursive_fib_n.8 = alloca i32, align 4
  %lv.inl_recursive_fib_result.9 = alloca i32, align 4
  %lv.inl_recursive_fib_n.9 = alloca i32, align 4
  %lv.inl_recursive_fib_result.10 = alloca i32, align 4
  %lv.inl_recursive_fib_n.10 = alloca i32, align 4
  %lv.inl_recursive_fib_result.11 = alloca i32, align 4
  %lv.inl_recursive_fib_n.11 = alloca i32, align 4
  %lv.inl_recursive_fib_result.12 = alloca i32, align 4
  %lv.inl_recursive_fib_n.12 = alloca i32, align 4
  %lv.inl_recursive_fib_result.13 = alloca i32, align 4
  %lv.inl_recursive_fib_n.13 = alloca i32, align 4
  %lv.inl_recursive_fib_result.14 = alloca i32, align 4
  %lv.inl_recursive_fib_n.14 = alloca i32, align 4
  %lv.inl_recursive_fib_result.15 = alloca i32, align 4
  %lv.inl_recursive_fib_n.15 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.1 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.2 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.3 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.4 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.5 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.6 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.7 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.8 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.9 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.10 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.11 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.12 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.13 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.14 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.15 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.16 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.17 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.18 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.19 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.20 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.21 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.22 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.23 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.24 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.25 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.26 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.27 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.28 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.29 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.30 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.31 = alloca i32, align 4
  %lv.inl_hoist_recursive_fib.32 = alloca i32, align 4
  %lv.inl_simple_inc_result = alloca i32, align 4
  %lv.inl_simple_inc_x = alloca i32, align 4
  %lv.inl_hoist_simple_inc = alloca i32, align 4
  %t0 = call i32 (i32, i32) @noinline_add(i32 3, i32 4)
  %t1 = icmp ne i32 %t0, 7
  %t2 = zext i1 %t1 to i32
  %t3 = icmp ne i32 %t2, 0
  br i1 %t3, label %block_11, label %block_12
block_11:
  ret i32 1
block_12:
  %t4 = call i32 (i32, i32) @default_mul(i32 3, i32 5)
  %t5 = icmp ne i32 %t4, 15
  %t6 = zext i1 %t5 to i32
  %t7 = icmp ne i32 %t6, 0
  br i1 %t7, label %block_13, label %block_14
block_13:
  ret i32 2
block_14:
  store i32 10, ptr %lv.inl_recursive_fib_n
  br label %block_24
block_24:
  %t8 = load i32, ptr %lv.inl_recursive_fib_n
  %t9 = icmp sle i32 %t8, 1
  %t10 = zext i1 %t9 to i32
  %t11 = icmp ne i32 %t10, 0
  br i1 %t11, label %block_25, label %block_26
block_25:
  %t12 = load i32, ptr %lv.inl_recursive_fib_n
  store i32 %t12, ptr %lv.inl_recursive_fib_result
  br label %block_23
block_26:
  %t13 = load i32, ptr %lv.inl_recursive_fib_n
  %t14 = sub i32 %t13, 1
  store i32 %t14, ptr %lv.inl_recursive_fib_n.1
  br label %block_28
block_28:
  %t15 = load i32, ptr %lv.inl_recursive_fib_n.1
  %t16 = icmp sle i32 %t15, 1
  %t17 = zext i1 %t16 to i32
  %t18 = icmp ne i32 %t17, 0
  br i1 %t18, label %block_29, label %block_30
block_29:
  %t19 = load i32, ptr %lv.inl_recursive_fib_n.1
  store i32 %t19, ptr %lv.inl_recursive_fib_result.1
  br label %block_27
block_30:
  %t20 = load i32, ptr %lv.inl_recursive_fib_n.1
  %t21 = sub i32 %t20, 1
  store i32 %t21, ptr %lv.inl_recursive_fib_n.2
  br label %block_32
block_32:
  %t22 = load i32, ptr %lv.inl_recursive_fib_n.2
  %t23 = icmp sle i32 %t22, 1
  %t24 = zext i1 %t23 to i32
  %t25 = icmp ne i32 %t24, 0
  br i1 %t25, label %block_33, label %block_34
block_33:
  %t26 = load i32, ptr %lv.inl_recursive_fib_n.2
  store i32 %t26, ptr %lv.inl_recursive_fib_result.2
  br label %block_31
block_34:
  %t27 = load i32, ptr %lv.inl_recursive_fib_n.2
  %t28 = sub i32 %t27, 1
  store i32 %t28, ptr %lv.inl_recursive_fib_n.3
  br label %block_36
block_36:
  %t29 = load i32, ptr %lv.inl_recursive_fib_n.3
  %t30 = icmp sle i32 %t29, 1
  %t31 = zext i1 %t30 to i32
  %t32 = icmp ne i32 %t31, 0
  br i1 %t32, label %block_37, label %block_38
block_37:
  %t33 = load i32, ptr %lv.inl_recursive_fib_n.3
  store i32 %t33, ptr %lv.inl_recursive_fib_result.3
  br label %block_35
block_38:
  %t34 = load i32, ptr %lv.inl_recursive_fib_n.3
  %t35 = sub i32 %t34, 1
  store i32 %t35, ptr %lv.inl_recursive_fib_n.4
  br label %block_40
block_40:
  %t36 = load i32, ptr %lv.inl_recursive_fib_n.4
  %t37 = icmp sle i32 %t36, 1
  %t38 = zext i1 %t37 to i32
  %t39 = icmp ne i32 %t38, 0
  br i1 %t39, label %block_41, label %block_42
block_41:
  %t40 = load i32, ptr %lv.inl_recursive_fib_n.4
  store i32 %t40, ptr %lv.inl_recursive_fib_result.4
  br label %block_39
block_42:
  %t41 = load i32, ptr %lv.inl_recursive_fib_n.4
  %t42 = sub i32 %t41, 1
  store i32 %t42, ptr %lv.inl_recursive_fib_n.5
  br label %block_44
block_44:
  %t43 = load i32, ptr %lv.inl_recursive_fib_n.5
  %t44 = icmp sle i32 %t43, 1
  %t45 = zext i1 %t44 to i32
  %t46 = icmp ne i32 %t45, 0
  br i1 %t46, label %block_45, label %block_46
block_45:
  %t47 = load i32, ptr %lv.inl_recursive_fib_n.5
  store i32 %t47, ptr %lv.inl_recursive_fib_result.5
  br label %block_43
block_46:
  %t48 = load i32, ptr %lv.inl_recursive_fib_n.5
  %t49 = sub i32 %t48, 1
  store i32 %t49, ptr %lv.inl_recursive_fib_n.6
  br label %block_48
block_48:
  %t50 = load i32, ptr %lv.inl_recursive_fib_n.6
  %t51 = icmp sle i32 %t50, 1
  %t52 = zext i1 %t51 to i32
  %t53 = icmp ne i32 %t52, 0
  br i1 %t53, label %block_49, label %block_50
block_49:
  %t54 = load i32, ptr %lv.inl_recursive_fib_n.6
  store i32 %t54, ptr %lv.inl_recursive_fib_result.6
  br label %block_47
block_50:
  %t55 = load i32, ptr %lv.inl_recursive_fib_n.6
  %t56 = sub i32 %t55, 1
  store i32 %t56, ptr %lv.inl_recursive_fib_n.7
  br label %block_52
block_52:
  %t57 = load i32, ptr %lv.inl_recursive_fib_n.7
  %t58 = icmp sle i32 %t57, 1
  %t59 = zext i1 %t58 to i32
  %t60 = icmp ne i32 %t59, 0
  br i1 %t60, label %block_53, label %block_54
block_53:
  %t61 = load i32, ptr %lv.inl_recursive_fib_n.7
  store i32 %t61, ptr %lv.inl_recursive_fib_result.7
  br label %block_51
block_54:
  %t62 = load i32, ptr %lv.inl_recursive_fib_n.7
  %t63 = sub i32 %t62, 1
  store i32 %t63, ptr %lv.inl_recursive_fib_n.8
  br label %block_56
block_56:
  %t64 = load i32, ptr %lv.inl_recursive_fib_n.8
  %t65 = icmp sle i32 %t64, 1
  %t66 = zext i1 %t65 to i32
  %t67 = icmp ne i32 %t66, 0
  br i1 %t67, label %block_57, label %block_58
block_57:
  %t68 = load i32, ptr %lv.inl_recursive_fib_n.8
  store i32 %t68, ptr %lv.inl_recursive_fib_result.8
  br label %block_55
block_58:
  %t69 = load i32, ptr %lv.inl_recursive_fib_n.8
  %t70 = sub i32 %t69, 1
  store i32 %t70, ptr %lv.inl_recursive_fib_n.9
  br label %block_60
block_60:
  %t71 = load i32, ptr %lv.inl_recursive_fib_n.9
  %t72 = icmp sle i32 %t71, 1
  %t73 = zext i1 %t72 to i32
  %t74 = icmp ne i32 %t73, 0
  br i1 %t74, label %block_61, label %block_62
block_61:
  %t75 = load i32, ptr %lv.inl_recursive_fib_n.9
  store i32 %t75, ptr %lv.inl_recursive_fib_result.9
  br label %block_59
block_62:
  %t76 = load i32, ptr %lv.inl_recursive_fib_n.9
  %t77 = sub i32 %t76, 1
  store i32 %t77, ptr %lv.inl_recursive_fib_n.10
  br label %block_64
block_64:
  %t78 = load i32, ptr %lv.inl_recursive_fib_n.10
  %t79 = icmp sle i32 %t78, 1
  %t80 = zext i1 %t79 to i32
  %t81 = icmp ne i32 %t80, 0
  br i1 %t81, label %block_65, label %block_66
block_65:
  %t82 = load i32, ptr %lv.inl_recursive_fib_n.10
  store i32 %t82, ptr %lv.inl_recursive_fib_result.10
  br label %block_63
block_66:
  %t83 = load i32, ptr %lv.inl_recursive_fib_n.10
  %t84 = sub i32 %t83, 1
  store i32 %t84, ptr %lv.inl_recursive_fib_n.11
  br label %block_68
block_68:
  %t85 = load i32, ptr %lv.inl_recursive_fib_n.11
  %t86 = icmp sle i32 %t85, 1
  %t87 = zext i1 %t86 to i32
  %t88 = icmp ne i32 %t87, 0
  br i1 %t88, label %block_69, label %block_70
block_69:
  %t89 = load i32, ptr %lv.inl_recursive_fib_n.11
  store i32 %t89, ptr %lv.inl_recursive_fib_result.11
  br label %block_67
block_70:
  %t90 = load i32, ptr %lv.inl_recursive_fib_n.11
  %t91 = sub i32 %t90, 1
  store i32 %t91, ptr %lv.inl_recursive_fib_n.12
  br label %block_72
block_72:
  %t92 = load i32, ptr %lv.inl_recursive_fib_n.12
  %t93 = icmp sle i32 %t92, 1
  %t94 = zext i1 %t93 to i32
  %t95 = icmp ne i32 %t94, 0
  br i1 %t95, label %block_73, label %block_74
block_73:
  %t96 = load i32, ptr %lv.inl_recursive_fib_n.12
  store i32 %t96, ptr %lv.inl_recursive_fib_result.12
  br label %block_71
block_74:
  %t97 = load i32, ptr %lv.inl_recursive_fib_n.12
  %t98 = sub i32 %t97, 1
  store i32 %t98, ptr %lv.inl_recursive_fib_n.13
  br label %block_76
block_76:
  %t99 = load i32, ptr %lv.inl_recursive_fib_n.13
  %t100 = icmp sle i32 %t99, 1
  %t101 = zext i1 %t100 to i32
  %t102 = icmp ne i32 %t101, 0
  br i1 %t102, label %block_77, label %block_78
block_77:
  %t103 = load i32, ptr %lv.inl_recursive_fib_n.13
  store i32 %t103, ptr %lv.inl_recursive_fib_result.13
  br label %block_75
block_78:
  %t104 = load i32, ptr %lv.inl_recursive_fib_n.13
  %t105 = sub i32 %t104, 1
  store i32 %t105, ptr %lv.inl_recursive_fib_n.14
  br label %block_80
block_80:
  %t106 = load i32, ptr %lv.inl_recursive_fib_n.14
  %t107 = icmp sle i32 %t106, 1
  %t108 = zext i1 %t107 to i32
  %t109 = icmp ne i32 %t108, 0
  br i1 %t109, label %block_81, label %block_82
block_81:
  %t110 = load i32, ptr %lv.inl_recursive_fib_n.14
  store i32 %t110, ptr %lv.inl_recursive_fib_result.14
  br label %block_79
block_82:
  %t111 = load i32, ptr %lv.inl_recursive_fib_n.14
  %t112 = sub i32 %t111, 1
  store i32 %t112, ptr %lv.inl_recursive_fib_n.15
  br label %block_84
block_84:
  %t113 = load i32, ptr %lv.inl_recursive_fib_n.15
  %t114 = icmp sle i32 %t113, 1
  %t115 = zext i1 %t114 to i32
  %t116 = icmp ne i32 %t115, 0
  br i1 %t116, label %block_85, label %block_86
block_85:
  %t117 = load i32, ptr %lv.inl_recursive_fib_n.15
  store i32 %t117, ptr %lv.inl_recursive_fib_result.15
  br label %block_83
block_86:
  %t118 = load i32, ptr %lv.inl_recursive_fib_n.15
  %t119 = sub i32 %t118, 1
  %t120 = call i32 (i32) @recursive_fib(i32 %t119)
  store i32 %t120, ptr %lv.inl_hoist_recursive_fib
  %t121 = load i32, ptr %lv.inl_recursive_fib_n.15
  %t122 = sub i32 %t121, 2
  %t123 = call i32 (i32) @recursive_fib(i32 %t122)
  store i32 %t123, ptr %lv.inl_hoist_recursive_fib.1
  %t124 = load i32, ptr %lv.inl_hoist_recursive_fib
  %t125 = load i32, ptr %lv.inl_hoist_recursive_fib.1
  %t126 = add i32 %t124, %t125
  store i32 %t126, ptr %lv.inl_recursive_fib_result.15
  br label %block_83
block_83:
  %t127 = load i32, ptr %lv.inl_recursive_fib_result.15
  store i32 %t127, ptr %lv.inl_hoist_recursive_fib.2
  %t128 = load i32, ptr %lv.inl_recursive_fib_n.14
  %t129 = sub i32 %t128, 2
  %t130 = call i32 (i32) @recursive_fib(i32 %t129)
  store i32 %t130, ptr %lv.inl_hoist_recursive_fib.3
  %t131 = load i32, ptr %lv.inl_hoist_recursive_fib.2
  %t132 = load i32, ptr %lv.inl_hoist_recursive_fib.3
  %t133 = add i32 %t131, %t132
  store i32 %t133, ptr %lv.inl_recursive_fib_result.14
  br label %block_79
block_79:
  %t134 = load i32, ptr %lv.inl_recursive_fib_result.14
  store i32 %t134, ptr %lv.inl_hoist_recursive_fib.4
  %t135 = load i32, ptr %lv.inl_recursive_fib_n.13
  %t136 = sub i32 %t135, 2
  %t137 = call i32 (i32) @recursive_fib(i32 %t136)
  store i32 %t137, ptr %lv.inl_hoist_recursive_fib.5
  %t138 = load i32, ptr %lv.inl_hoist_recursive_fib.4
  %t139 = load i32, ptr %lv.inl_hoist_recursive_fib.5
  %t140 = add i32 %t138, %t139
  store i32 %t140, ptr %lv.inl_recursive_fib_result.13
  br label %block_75
block_75:
  %t141 = load i32, ptr %lv.inl_recursive_fib_result.13
  store i32 %t141, ptr %lv.inl_hoist_recursive_fib.6
  %t142 = load i32, ptr %lv.inl_recursive_fib_n.12
  %t143 = sub i32 %t142, 2
  %t144 = call i32 (i32) @recursive_fib(i32 %t143)
  store i32 %t144, ptr %lv.inl_hoist_recursive_fib.7
  %t145 = load i32, ptr %lv.inl_hoist_recursive_fib.6
  %t146 = load i32, ptr %lv.inl_hoist_recursive_fib.7
  %t147 = add i32 %t145, %t146
  store i32 %t147, ptr %lv.inl_recursive_fib_result.12
  br label %block_71
block_71:
  %t148 = load i32, ptr %lv.inl_recursive_fib_result.12
  store i32 %t148, ptr %lv.inl_hoist_recursive_fib.8
  %t149 = load i32, ptr %lv.inl_recursive_fib_n.11
  %t150 = sub i32 %t149, 2
  %t151 = call i32 (i32) @recursive_fib(i32 %t150)
  store i32 %t151, ptr %lv.inl_hoist_recursive_fib.9
  %t152 = load i32, ptr %lv.inl_hoist_recursive_fib.8
  %t153 = load i32, ptr %lv.inl_hoist_recursive_fib.9
  %t154 = add i32 %t152, %t153
  store i32 %t154, ptr %lv.inl_recursive_fib_result.11
  br label %block_67
block_67:
  %t155 = load i32, ptr %lv.inl_recursive_fib_result.11
  store i32 %t155, ptr %lv.inl_hoist_recursive_fib.10
  %t156 = load i32, ptr %lv.inl_recursive_fib_n.10
  %t157 = sub i32 %t156, 2
  %t158 = call i32 (i32) @recursive_fib(i32 %t157)
  store i32 %t158, ptr %lv.inl_hoist_recursive_fib.11
  %t159 = load i32, ptr %lv.inl_hoist_recursive_fib.10
  %t160 = load i32, ptr %lv.inl_hoist_recursive_fib.11
  %t161 = add i32 %t159, %t160
  store i32 %t161, ptr %lv.inl_recursive_fib_result.10
  br label %block_63
block_63:
  %t162 = load i32, ptr %lv.inl_recursive_fib_result.10
  store i32 %t162, ptr %lv.inl_hoist_recursive_fib.12
  %t163 = load i32, ptr %lv.inl_recursive_fib_n.9
  %t164 = sub i32 %t163, 2
  %t165 = call i32 (i32) @recursive_fib(i32 %t164)
  store i32 %t165, ptr %lv.inl_hoist_recursive_fib.13
  %t166 = load i32, ptr %lv.inl_hoist_recursive_fib.12
  %t167 = load i32, ptr %lv.inl_hoist_recursive_fib.13
  %t168 = add i32 %t166, %t167
  store i32 %t168, ptr %lv.inl_recursive_fib_result.9
  br label %block_59
block_59:
  %t169 = load i32, ptr %lv.inl_recursive_fib_result.9
  store i32 %t169, ptr %lv.inl_hoist_recursive_fib.14
  %t170 = load i32, ptr %lv.inl_recursive_fib_n.8
  %t171 = sub i32 %t170, 2
  %t172 = call i32 (i32) @recursive_fib(i32 %t171)
  store i32 %t172, ptr %lv.inl_hoist_recursive_fib.15
  %t173 = load i32, ptr %lv.inl_hoist_recursive_fib.14
  %t174 = load i32, ptr %lv.inl_hoist_recursive_fib.15
  %t175 = add i32 %t173, %t174
  store i32 %t175, ptr %lv.inl_recursive_fib_result.8
  br label %block_55
block_55:
  %t176 = load i32, ptr %lv.inl_recursive_fib_result.8
  store i32 %t176, ptr %lv.inl_hoist_recursive_fib.16
  %t177 = load i32, ptr %lv.inl_recursive_fib_n.7
  %t178 = sub i32 %t177, 2
  %t179 = call i32 (i32) @recursive_fib(i32 %t178)
  store i32 %t179, ptr %lv.inl_hoist_recursive_fib.17
  %t180 = load i32, ptr %lv.inl_hoist_recursive_fib.16
  %t181 = load i32, ptr %lv.inl_hoist_recursive_fib.17
  %t182 = add i32 %t180, %t181
  store i32 %t182, ptr %lv.inl_recursive_fib_result.7
  br label %block_51
block_51:
  %t183 = load i32, ptr %lv.inl_recursive_fib_result.7
  store i32 %t183, ptr %lv.inl_hoist_recursive_fib.18
  %t184 = load i32, ptr %lv.inl_recursive_fib_n.6
  %t185 = sub i32 %t184, 2
  %t186 = call i32 (i32) @recursive_fib(i32 %t185)
  store i32 %t186, ptr %lv.inl_hoist_recursive_fib.19
  %t187 = load i32, ptr %lv.inl_hoist_recursive_fib.18
  %t188 = load i32, ptr %lv.inl_hoist_recursive_fib.19
  %t189 = add i32 %t187, %t188
  store i32 %t189, ptr %lv.inl_recursive_fib_result.6
  br label %block_47
block_47:
  %t190 = load i32, ptr %lv.inl_recursive_fib_result.6
  store i32 %t190, ptr %lv.inl_hoist_recursive_fib.20
  %t191 = load i32, ptr %lv.inl_recursive_fib_n.5
  %t192 = sub i32 %t191, 2
  %t193 = call i32 (i32) @recursive_fib(i32 %t192)
  store i32 %t193, ptr %lv.inl_hoist_recursive_fib.21
  %t194 = load i32, ptr %lv.inl_hoist_recursive_fib.20
  %t195 = load i32, ptr %lv.inl_hoist_recursive_fib.21
  %t196 = add i32 %t194, %t195
  store i32 %t196, ptr %lv.inl_recursive_fib_result.5
  br label %block_43
block_43:
  %t197 = load i32, ptr %lv.inl_recursive_fib_result.5
  store i32 %t197, ptr %lv.inl_hoist_recursive_fib.22
  %t198 = load i32, ptr %lv.inl_recursive_fib_n.4
  %t199 = sub i32 %t198, 2
  %t200 = call i32 (i32) @recursive_fib(i32 %t199)
  store i32 %t200, ptr %lv.inl_hoist_recursive_fib.23
  %t201 = load i32, ptr %lv.inl_hoist_recursive_fib.22
  %t202 = load i32, ptr %lv.inl_hoist_recursive_fib.23
  %t203 = add i32 %t201, %t202
  store i32 %t203, ptr %lv.inl_recursive_fib_result.4
  br label %block_39
block_39:
  %t204 = load i32, ptr %lv.inl_recursive_fib_result.4
  store i32 %t204, ptr %lv.inl_hoist_recursive_fib.24
  %t205 = load i32, ptr %lv.inl_recursive_fib_n.3
  %t206 = sub i32 %t205, 2
  %t207 = call i32 (i32) @recursive_fib(i32 %t206)
  store i32 %t207, ptr %lv.inl_hoist_recursive_fib.25
  %t208 = load i32, ptr %lv.inl_hoist_recursive_fib.24
  %t209 = load i32, ptr %lv.inl_hoist_recursive_fib.25
  %t210 = add i32 %t208, %t209
  store i32 %t210, ptr %lv.inl_recursive_fib_result.3
  br label %block_35
block_35:
  %t211 = load i32, ptr %lv.inl_recursive_fib_result.3
  store i32 %t211, ptr %lv.inl_hoist_recursive_fib.26
  %t212 = load i32, ptr %lv.inl_recursive_fib_n.2
  %t213 = sub i32 %t212, 2
  %t214 = call i32 (i32) @recursive_fib(i32 %t213)
  store i32 %t214, ptr %lv.inl_hoist_recursive_fib.27
  %t215 = load i32, ptr %lv.inl_hoist_recursive_fib.26
  %t216 = load i32, ptr %lv.inl_hoist_recursive_fib.27
  %t217 = add i32 %t215, %t216
  store i32 %t217, ptr %lv.inl_recursive_fib_result.2
  br label %block_31
block_31:
  %t218 = load i32, ptr %lv.inl_recursive_fib_result.2
  store i32 %t218, ptr %lv.inl_hoist_recursive_fib.28
  %t219 = load i32, ptr %lv.inl_recursive_fib_n.1
  %t220 = sub i32 %t219, 2
  %t221 = call i32 (i32) @recursive_fib(i32 %t220)
  store i32 %t221, ptr %lv.inl_hoist_recursive_fib.29
  %t222 = load i32, ptr %lv.inl_hoist_recursive_fib.28
  %t223 = load i32, ptr %lv.inl_hoist_recursive_fib.29
  %t224 = add i32 %t222, %t223
  store i32 %t224, ptr %lv.inl_recursive_fib_result.1
  br label %block_27
block_27:
  %t225 = load i32, ptr %lv.inl_recursive_fib_result.1
  store i32 %t225, ptr %lv.inl_hoist_recursive_fib.30
  %t226 = load i32, ptr %lv.inl_recursive_fib_n
  %t227 = sub i32 %t226, 2
  %t228 = call i32 (i32) @recursive_fib(i32 %t227)
  store i32 %t228, ptr %lv.inl_hoist_recursive_fib.31
  %t229 = load i32, ptr %lv.inl_hoist_recursive_fib.30
  %t230 = load i32, ptr %lv.inl_hoist_recursive_fib.31
  %t231 = add i32 %t229, %t230
  store i32 %t231, ptr %lv.inl_recursive_fib_result
  br label %block_23
block_23:
  %t232 = load i32, ptr %lv.inl_recursive_fib_result
  store i32 %t232, ptr %lv.inl_hoist_recursive_fib.32
  %t233 = load i32, ptr %lv.inl_hoist_recursive_fib.32
  %t234 = icmp ne i32 %t233, 55
  %t235 = zext i1 %t234 to i32
  %t236 = icmp ne i32 %t235, 0
  br i1 %t236, label %block_15, label %block_16
block_15:
  ret i32 3
block_16:
  %t237 = call i32 (i32, ...) @variadic_sum(i32 3, i32 10, i32 20, i32 30)
  %t238 = icmp ne i32 %t237, 60
  %t239 = zext i1 %t238 to i32
  %t240 = icmp ne i32 %t239, 0
  br i1 %t240, label %block_17, label %block_18
block_17:
  ret i32 4
block_18:
  %t241 = call i32 (i32) @noinline_wins(i32 5)
  %t242 = icmp ne i32 %t241, 105
  %t243 = zext i1 %t242 to i32
  %t244 = icmp ne i32 %t243, 0
  br i1 %t244, label %block_19, label %block_20
block_19:
  ret i32 5
block_20:
  store i32 41, ptr %lv.inl_simple_inc_x
  br label %block_88
block_88:
  %t245 = load i32, ptr %lv.inl_simple_inc_x
  %t246 = add i32 %t245, 1
  store i32 %t246, ptr %lv.inl_simple_inc_result
  br label %block_87
block_87:
  %t247 = load i32, ptr %lv.inl_simple_inc_result
  store i32 %t247, ptr %lv.inl_hoist_simple_inc
  %t248 = load i32, ptr %lv.inl_hoist_simple_inc
  %t249 = icmp ne i32 %t248, 42
  %t250 = zext i1 %t249 to i32
  %t251 = icmp ne i32 %t250, 0
  br i1 %t251, label %block_21, label %block_22
block_21:
  ret i32 6
block_22:
  ret i32 0
}

