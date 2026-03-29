target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag_ = type { ptr, ptr, ptr, i32, i32 }
@one = global i32 1, align 4

declare i32 @abort(...)

define i32 @main(i32 %p.argc, ptr %p.argv)
{
entry:
  %lv.v0 = alloca <8 x i16>, align 2
  %lv.v1 = alloca <8 x i16>, align 2
  %lv.f0 = alloca <4 x float>, align 4
  %lv.f1 = alloca <4 x float>, align 4
  %lv.f2 = alloca <4 x float>, align 4
  %lv.d0 = alloca <2 x double>, align 8
  %lv.d1 = alloca <2 x double>, align 8
  %lv.d2 = alloca <2 x double>, align 8
  %lv.__i = alloca i32, align 4
  %lv.__i.1 = alloca i32, align 4
  %lv.__i.2 = alloca i32, align 4
  %lv.__i.3 = alloca i32, align 4
  %lv.__i.4 = alloca i32, align 4
  %lv.__i.5 = alloca i32, align 4
  %lv.__i.6 = alloca i32, align 4
  %lv.__i.7 = alloca i32, align 4
  %lv.__i.8 = alloca i32, align 4
  %lv.__i.9 = alloca i32, align 4
  %lv.__i.10 = alloca i32, align 4
  %lv.__i.11 = alloca i32, align 4
  %lv.__i.12 = alloca i32, align 4
  %lv.__i.13 = alloca i32, align 4
  %lv.__i.14 = alloca i32, align 4
  %lv.__i.15 = alloca i32, align 4
  %lv.__i.16 = alloca i32, align 4
  %lv.__i.17 = alloca i32, align 4
  %lv._clit_ = alloca <4 x float>, align 4
  %lv.__i.18 = alloca i32, align 4
  %lv._clit_.1 = alloca <4 x float>, align 4
  %lv.__i.19 = alloca i32, align 4
  %lv._clit_.2 = alloca <4 x float>, align 4
  %lv.__i.20 = alloca i32, align 4
  %lv._clit_.3 = alloca <4 x float>, align 4
  %lv.__i.21 = alloca i32, align 4
  %lv._clit_.4 = alloca <4 x float>, align 4
  %lv.__i.22 = alloca i32, align 4
  %lv._clit_.5 = alloca <4 x float>, align 4
  %lv.__i.23 = alloca i32, align 4
  %lv._clit_.6 = alloca <4 x float>, align 4
  %lv.__i.24 = alloca i32, align 4
  %lv._clit_.7 = alloca <4 x float>, align 4
  %lv.__i.25 = alloca i32, align 4
  %lv._clit_.8 = alloca <2 x double>, align 8
  %lv.__i.26 = alloca i32, align 4
  %lv._clit_.9 = alloca <2 x double>, align 8
  %lv.__i.27 = alloca i32, align 4
  %lv._clit_.10 = alloca <2 x double>, align 8
  %lv.__i.28 = alloca i32, align 4
  %lv._clit_.11 = alloca <2 x double>, align 8
  %lv.__i.29 = alloca i32, align 4
  %lv._clit_.12 = alloca <2 x double>, align 8
  %lv.__i.30 = alloca i32, align 4
  %lv._clit_.13 = alloca <2 x double>, align 8
  %lv.__i.31 = alloca i32, align 4
  %lv._clit_.14 = alloca <2 x double>, align 8
  %lv.__i.32 = alloca i32, align 4
  %lv._clit_.15 = alloca <2 x double>, align 8
  %lv.__i.33 = alloca i32, align 4
  store <8 x i16> zeroinitializer, ptr %lv.v0
  %t0 = load i32, ptr @one
  %t1 = sext i32 0 to i64
  %t2 = getelementptr i16, ptr %lv.v0, i64 %t1
  %t3 = trunc i32 %t0 to i16
  store i16 %t3, ptr %t2
  %t4 = sext i32 1 to i64
  %t5 = getelementptr i16, ptr %lv.v0, i64 %t4
  %t6 = trunc i32 1 to i16
  store i16 %t6, ptr %t5
  %t7 = sext i32 2 to i64
  %t8 = getelementptr i16, ptr %lv.v0, i64 %t7
  %t9 = trunc i32 2 to i16
  store i16 %t9, ptr %t8
  %t10 = sext i32 3 to i64
  %t11 = getelementptr i16, ptr %lv.v0, i64 %t10
  %t12 = trunc i32 3 to i16
  store i16 %t12, ptr %t11
  %t13 = sext i32 4 to i64
  %t14 = getelementptr i16, ptr %lv.v0, i64 %t13
  %t15 = trunc i32 4 to i16
  store i16 %t15, ptr %t14
  %t16 = sext i32 5 to i64
  %t17 = getelementptr i16, ptr %lv.v0, i64 %t16
  %t18 = trunc i32 5 to i16
  store i16 %t18, ptr %t17
  %t19 = sext i32 6 to i64
  %t20 = getelementptr i16, ptr %lv.v0, i64 %t19
  %t21 = trunc i32 6 to i16
  store i16 %t21, ptr %t20
  %t22 = sext i32 7 to i64
  %t23 = getelementptr i16, ptr %lv.v0, i64 %t22
  %t24 = trunc i32 7 to i16
  store i16 %t24, ptr %t23
  store <4 x float> zeroinitializer, ptr %lv.f0
  %t25 = sext i32 0 to i64
  %t26 = getelementptr float, ptr %lv.f0, i64 %t25
  %t27 = fptrunc double 0x3FF0000000000000 to float
  store float %t27, ptr %t26
  %t28 = sext i32 1 to i64
  %t29 = getelementptr float, ptr %lv.f0, i64 %t28
  %t30 = fptrunc double 0x4000000000000000 to float
  store float %t30, ptr %t29
  %t31 = sext i32 2 to i64
  %t32 = getelementptr float, ptr %lv.f0, i64 %t31
  %t33 = fptrunc double 0x4008000000000000 to float
  store float %t33, ptr %t32
  %t34 = sext i32 3 to i64
  %t35 = getelementptr float, ptr %lv.f0, i64 %t34
  %t36 = fptrunc double 0x4010000000000000 to float
  store float %t36, ptr %t35
  store <2 x double> zeroinitializer, ptr %lv.d0
  %t37 = sext i32 0 to i64
  %t38 = getelementptr double, ptr %lv.d0, i64 %t37
  store double 0x3FF0000000000000, ptr %t38
  %t39 = sext i32 1 to i64
  %t40 = getelementptr double, ptr %lv.d0, i64 %t39
  store double 0x4000000000000000, ptr %t40
  %t41 = load <8 x i16>, ptr %lv.v0
  %t42 = trunc i32 2 to i16
  %t43 = insertelement <8 x i16> poison, i16 %t42, i64 0
  %t44 = shufflevector <8 x i16> %t43, <8 x i16> poison, <8 x i32> zeroinitializer
  %t45 = add <8 x i16> %t44, %t41
  store <8 x i16> %t45, ptr %lv.v1
  br label %block_1
block_1:
  store i32 0, ptr %lv.__i
  br label %for.cond.4
for.cond.4:
  %t46 = load i32, ptr %lv.__i
  %t47 = icmp slt i32 %t46, 8
  %t48 = zext i1 %t47 to i32
  %t49 = icmp ne i32 %t48, 0
  br i1 %t49, label %block_4, label %block_5
for.latch.4:
  %t50 = load i32, ptr %lv.__i
  %t51 = add i32 %t50, 1
  store i32 %t51, ptr %lv.__i
  br label %for.cond.4
block_2:
  br label %dowhile.cond.1
dowhile.cond.1:
  %t52 = icmp ne i32 0, 0
  br i1 %t52, label %block_1, label %block_3
block_3:
  %t53 = load <8 x i16>, ptr %lv.v0
  %t54 = trunc i32 2 to i16
  %t55 = insertelement <8 x i16> poison, i16 %t54, i64 0
  %t56 = shufflevector <8 x i16> %t55, <8 x i16> poison, <8 x i32> zeroinitializer
  %t57 = sub <8 x i16> %t56, %t53
  store <8 x i16> %t57, ptr %lv.v1
  br label %block_8
block_4:
  %t58 = load i32, ptr %lv.__i
  %t59 = sext i32 %t58 to i64
  %t60 = getelementptr i16, ptr %lv.v1, i64 %t59
  %t61 = load i16, ptr %t60
  %t62 = load i32, ptr %lv.__i
  %t63 = sext i32 %t62 to i64
  %t64 = getelementptr i16, ptr %lv.v0, i64 %t63
  %t65 = load i16, ptr %t64
  %t66 = sext i16 %t65 to i32
  %t67 = add i32 2, %t66
  %t68 = sext i16 %t61 to i32
  %t69 = icmp ne i32 %t68, %t67
  %t70 = zext i1 %t69 to i32
  %t71 = icmp ne i32 %t70, 0
  br i1 %t71, label %block_6, label %block_7
block_5:
  br label %block_2
block_6:
  %t72 = call i32 @abort()
  br label %block_7
block_7:
  br label %for.latch.4
block_8:
  store i32 0, ptr %lv.__i.1
  br label %for.cond.11
for.cond.11:
  %t73 = load i32, ptr %lv.__i.1
  %t74 = icmp slt i32 %t73, 8
  %t75 = zext i1 %t74 to i32
  %t76 = icmp ne i32 %t75, 0
  br i1 %t76, label %block_11, label %block_12
for.latch.11:
  %t77 = load i32, ptr %lv.__i.1
  %t78 = add i32 %t77, 1
  store i32 %t78, ptr %lv.__i.1
  br label %for.cond.11
block_9:
  br label %dowhile.cond.8
dowhile.cond.8:
  %t79 = icmp ne i32 0, 0
  br i1 %t79, label %block_8, label %block_10
block_10:
  %t80 = load <8 x i16>, ptr %lv.v0
  %t81 = trunc i32 2 to i16
  %t82 = insertelement <8 x i16> poison, i16 %t81, i64 0
  %t83 = shufflevector <8 x i16> %t82, <8 x i16> poison, <8 x i32> zeroinitializer
  %t84 = mul <8 x i16> %t83, %t80
  store <8 x i16> %t84, ptr %lv.v1
  br label %block_15
block_11:
  %t85 = load i32, ptr %lv.__i.1
  %t86 = sext i32 %t85 to i64
  %t87 = getelementptr i16, ptr %lv.v1, i64 %t86
  %t88 = load i16, ptr %t87
  %t89 = load i32, ptr %lv.__i.1
  %t90 = sext i32 %t89 to i64
  %t91 = getelementptr i16, ptr %lv.v0, i64 %t90
  %t92 = load i16, ptr %t91
  %t93 = sext i16 %t92 to i32
  %t94 = sub i32 2, %t93
  %t95 = sext i16 %t88 to i32
  %t96 = icmp ne i32 %t95, %t94
  %t97 = zext i1 %t96 to i32
  %t98 = icmp ne i32 %t97, 0
  br i1 %t98, label %block_13, label %block_14
block_12:
  br label %block_9
block_13:
  %t99 = call i32 @abort()
  br label %block_14
block_14:
  br label %for.latch.11
block_15:
  store i32 0, ptr %lv.__i.2
  br label %for.cond.18
for.cond.18:
  %t100 = load i32, ptr %lv.__i.2
  %t101 = icmp slt i32 %t100, 8
  %t102 = zext i1 %t101 to i32
  %t103 = icmp ne i32 %t102, 0
  br i1 %t103, label %block_18, label %block_19
for.latch.18:
  %t104 = load i32, ptr %lv.__i.2
  %t105 = add i32 %t104, 1
  store i32 %t105, ptr %lv.__i.2
  br label %for.cond.18
block_16:
  br label %dowhile.cond.15
dowhile.cond.15:
  %t106 = icmp ne i32 0, 0
  br i1 %t106, label %block_15, label %block_17
block_17:
  %t107 = load <8 x i16>, ptr %lv.v0
  %t108 = trunc i32 2 to i16
  %t109 = insertelement <8 x i16> poison, i16 %t108, i64 0
  %t110 = shufflevector <8 x i16> %t109, <8 x i16> poison, <8 x i32> zeroinitializer
  %t111 = sdiv <8 x i16> %t110, %t107
  store <8 x i16> %t111, ptr %lv.v1
  br label %block_22
block_18:
  %t112 = load i32, ptr %lv.__i.2
  %t113 = sext i32 %t112 to i64
  %t114 = getelementptr i16, ptr %lv.v1, i64 %t113
  %t115 = load i16, ptr %t114
  %t116 = load i32, ptr %lv.__i.2
  %t117 = sext i32 %t116 to i64
  %t118 = getelementptr i16, ptr %lv.v0, i64 %t117
  %t119 = load i16, ptr %t118
  %t120 = sext i16 %t119 to i32
  %t121 = mul i32 2, %t120
  %t122 = sext i16 %t115 to i32
  %t123 = icmp ne i32 %t122, %t121
  %t124 = zext i1 %t123 to i32
  %t125 = icmp ne i32 %t124, 0
  br i1 %t125, label %block_20, label %block_21
block_19:
  br label %block_16
block_20:
  %t126 = call i32 @abort()
  br label %block_21
block_21:
  br label %for.latch.18
block_22:
  store i32 0, ptr %lv.__i.3
  br label %for.cond.25
for.cond.25:
  %t127 = load i32, ptr %lv.__i.3
  %t128 = icmp slt i32 %t127, 8
  %t129 = zext i1 %t128 to i32
  %t130 = icmp ne i32 %t129, 0
  br i1 %t130, label %block_25, label %block_26
for.latch.25:
  %t131 = load i32, ptr %lv.__i.3
  %t132 = add i32 %t131, 1
  store i32 %t132, ptr %lv.__i.3
  br label %for.cond.25
block_23:
  br label %dowhile.cond.22
dowhile.cond.22:
  %t133 = icmp ne i32 0, 0
  br i1 %t133, label %block_22, label %block_24
block_24:
  %t134 = load <8 x i16>, ptr %lv.v0
  %t135 = trunc i32 2 to i16
  %t136 = insertelement <8 x i16> poison, i16 %t135, i64 0
  %t137 = shufflevector <8 x i16> %t136, <8 x i16> poison, <8 x i32> zeroinitializer
  %t138 = srem <8 x i16> %t137, %t134
  store <8 x i16> %t138, ptr %lv.v1
  br label %block_29
block_25:
  %t139 = load i32, ptr %lv.__i.3
  %t140 = sext i32 %t139 to i64
  %t141 = getelementptr i16, ptr %lv.v1, i64 %t140
  %t142 = load i16, ptr %t141
  %t143 = load i32, ptr %lv.__i.3
  %t144 = sext i32 %t143 to i64
  %t145 = getelementptr i16, ptr %lv.v0, i64 %t144
  %t146 = load i16, ptr %t145
  %t147 = sext i16 %t146 to i32
  %t148 = sdiv i32 2, %t147
  %t149 = sext i16 %t142 to i32
  %t150 = icmp ne i32 %t149, %t148
  %t151 = zext i1 %t150 to i32
  %t152 = icmp ne i32 %t151, 0
  br i1 %t152, label %block_27, label %block_28
block_26:
  br label %block_23
block_27:
  %t153 = call i32 @abort()
  br label %block_28
block_28:
  br label %for.latch.25
block_29:
  store i32 0, ptr %lv.__i.4
  br label %for.cond.32
for.cond.32:
  %t154 = load i32, ptr %lv.__i.4
  %t155 = icmp slt i32 %t154, 8
  %t156 = zext i1 %t155 to i32
  %t157 = icmp ne i32 %t156, 0
  br i1 %t157, label %block_32, label %block_33
for.latch.32:
  %t158 = load i32, ptr %lv.__i.4
  %t159 = add i32 %t158, 1
  store i32 %t159, ptr %lv.__i.4
  br label %for.cond.32
block_30:
  br label %dowhile.cond.29
dowhile.cond.29:
  %t160 = icmp ne i32 0, 0
  br i1 %t160, label %block_29, label %block_31
block_31:
  %t161 = load <8 x i16>, ptr %lv.v0
  %t162 = trunc i32 2 to i16
  %t163 = insertelement <8 x i16> poison, i16 %t162, i64 0
  %t164 = shufflevector <8 x i16> %t163, <8 x i16> poison, <8 x i32> zeroinitializer
  %t165 = xor <8 x i16> %t164, %t161
  store <8 x i16> %t165, ptr %lv.v1
  br label %block_36
block_32:
  %t166 = load i32, ptr %lv.__i.4
  %t167 = sext i32 %t166 to i64
  %t168 = getelementptr i16, ptr %lv.v1, i64 %t167
  %t169 = load i16, ptr %t168
  %t170 = load i32, ptr %lv.__i.4
  %t171 = sext i32 %t170 to i64
  %t172 = getelementptr i16, ptr %lv.v0, i64 %t171
  %t173 = load i16, ptr %t172
  %t174 = sext i16 %t173 to i32
  %t175 = srem i32 2, %t174
  %t176 = sext i16 %t169 to i32
  %t177 = icmp ne i32 %t176, %t175
  %t178 = zext i1 %t177 to i32
  %t179 = icmp ne i32 %t178, 0
  br i1 %t179, label %block_34, label %block_35
block_33:
  br label %block_30
block_34:
  %t180 = call i32 @abort()
  br label %block_35
block_35:
  br label %for.latch.32
block_36:
  store i32 0, ptr %lv.__i.5
  br label %for.cond.39
for.cond.39:
  %t181 = load i32, ptr %lv.__i.5
  %t182 = icmp slt i32 %t181, 8
  %t183 = zext i1 %t182 to i32
  %t184 = icmp ne i32 %t183, 0
  br i1 %t184, label %block_39, label %block_40
for.latch.39:
  %t185 = load i32, ptr %lv.__i.5
  %t186 = add i32 %t185, 1
  store i32 %t186, ptr %lv.__i.5
  br label %for.cond.39
block_37:
  br label %dowhile.cond.36
dowhile.cond.36:
  %t187 = icmp ne i32 0, 0
  br i1 %t187, label %block_36, label %block_38
block_38:
  %t188 = load <8 x i16>, ptr %lv.v0
  %t189 = trunc i32 2 to i16
  %t190 = insertelement <8 x i16> poison, i16 %t189, i64 0
  %t191 = shufflevector <8 x i16> %t190, <8 x i16> poison, <8 x i32> zeroinitializer
  %t192 = and <8 x i16> %t191, %t188
  store <8 x i16> %t192, ptr %lv.v1
  br label %block_43
block_39:
  %t193 = load i32, ptr %lv.__i.5
  %t194 = sext i32 %t193 to i64
  %t195 = getelementptr i16, ptr %lv.v1, i64 %t194
  %t196 = load i16, ptr %t195
  %t197 = load i32, ptr %lv.__i.5
  %t198 = sext i32 %t197 to i64
  %t199 = getelementptr i16, ptr %lv.v0, i64 %t198
  %t200 = load i16, ptr %t199
  %t201 = sext i16 %t200 to i32
  %t202 = xor i32 2, %t201
  %t203 = sext i16 %t196 to i32
  %t204 = icmp ne i32 %t203, %t202
  %t205 = zext i1 %t204 to i32
  %t206 = icmp ne i32 %t205, 0
  br i1 %t206, label %block_41, label %block_42
block_40:
  br label %block_37
block_41:
  %t207 = call i32 @abort()
  br label %block_42
block_42:
  br label %for.latch.39
block_43:
  store i32 0, ptr %lv.__i.6
  br label %for.cond.46
for.cond.46:
  %t208 = load i32, ptr %lv.__i.6
  %t209 = icmp slt i32 %t208, 8
  %t210 = zext i1 %t209 to i32
  %t211 = icmp ne i32 %t210, 0
  br i1 %t211, label %block_46, label %block_47
for.latch.46:
  %t212 = load i32, ptr %lv.__i.6
  %t213 = add i32 %t212, 1
  store i32 %t213, ptr %lv.__i.6
  br label %for.cond.46
block_44:
  br label %dowhile.cond.43
dowhile.cond.43:
  %t214 = icmp ne i32 0, 0
  br i1 %t214, label %block_43, label %block_45
block_45:
  %t215 = load <8 x i16>, ptr %lv.v0
  %t216 = trunc i32 2 to i16
  %t217 = insertelement <8 x i16> poison, i16 %t216, i64 0
  %t218 = shufflevector <8 x i16> %t217, <8 x i16> poison, <8 x i32> zeroinitializer
  %t219 = or <8 x i16> %t218, %t215
  store <8 x i16> %t219, ptr %lv.v1
  br label %block_50
block_46:
  %t220 = load i32, ptr %lv.__i.6
  %t221 = sext i32 %t220 to i64
  %t222 = getelementptr i16, ptr %lv.v1, i64 %t221
  %t223 = load i16, ptr %t222
  %t224 = load i32, ptr %lv.__i.6
  %t225 = sext i32 %t224 to i64
  %t226 = getelementptr i16, ptr %lv.v0, i64 %t225
  %t227 = load i16, ptr %t226
  %t228 = sext i16 %t227 to i32
  %t229 = and i32 2, %t228
  %t230 = sext i16 %t223 to i32
  %t231 = icmp ne i32 %t230, %t229
  %t232 = zext i1 %t231 to i32
  %t233 = icmp ne i32 %t232, 0
  br i1 %t233, label %block_48, label %block_49
block_47:
  br label %block_44
block_48:
  %t234 = call i32 @abort()
  br label %block_49
block_49:
  br label %for.latch.46
block_50:
  store i32 0, ptr %lv.__i.7
  br label %for.cond.53
for.cond.53:
  %t235 = load i32, ptr %lv.__i.7
  %t236 = icmp slt i32 %t235, 8
  %t237 = zext i1 %t236 to i32
  %t238 = icmp ne i32 %t237, 0
  br i1 %t238, label %block_53, label %block_54
for.latch.53:
  %t239 = load i32, ptr %lv.__i.7
  %t240 = add i32 %t239, 1
  store i32 %t240, ptr %lv.__i.7
  br label %for.cond.53
block_51:
  br label %dowhile.cond.50
dowhile.cond.50:
  %t241 = icmp ne i32 0, 0
  br i1 %t241, label %block_50, label %block_52
block_52:
  %t242 = load <8 x i16>, ptr %lv.v0
  %t243 = trunc i32 2 to i16
  %t244 = insertelement <8 x i16> poison, i16 %t243, i64 0
  %t245 = shufflevector <8 x i16> %t244, <8 x i16> poison, <8 x i32> zeroinitializer
  %t246 = shl <8 x i16> %t245, %t242
  store <8 x i16> %t246, ptr %lv.v1
  br label %block_57
block_53:
  %t247 = load i32, ptr %lv.__i.7
  %t248 = sext i32 %t247 to i64
  %t249 = getelementptr i16, ptr %lv.v1, i64 %t248
  %t250 = load i16, ptr %t249
  %t251 = load i32, ptr %lv.__i.7
  %t252 = sext i32 %t251 to i64
  %t253 = getelementptr i16, ptr %lv.v0, i64 %t252
  %t254 = load i16, ptr %t253
  %t255 = sext i16 %t254 to i32
  %t256 = or i32 2, %t255
  %t257 = sext i16 %t250 to i32
  %t258 = icmp ne i32 %t257, %t256
  %t259 = zext i1 %t258 to i32
  %t260 = icmp ne i32 %t259, 0
  br i1 %t260, label %block_55, label %block_56
block_54:
  br label %block_51
block_55:
  %t261 = call i32 @abort()
  br label %block_56
block_56:
  br label %for.latch.53
block_57:
  store i32 0, ptr %lv.__i.8
  br label %for.cond.60
for.cond.60:
  %t262 = load i32, ptr %lv.__i.8
  %t263 = icmp slt i32 %t262, 8
  %t264 = zext i1 %t263 to i32
  %t265 = icmp ne i32 %t264, 0
  br i1 %t265, label %block_60, label %block_61
for.latch.60:
  %t266 = load i32, ptr %lv.__i.8
  %t267 = add i32 %t266, 1
  store i32 %t267, ptr %lv.__i.8
  br label %for.cond.60
block_58:
  br label %dowhile.cond.57
dowhile.cond.57:
  %t268 = icmp ne i32 0, 0
  br i1 %t268, label %block_57, label %block_59
block_59:
  %t269 = load <8 x i16>, ptr %lv.v0
  %t270 = trunc i32 2 to i16
  %t271 = insertelement <8 x i16> poison, i16 %t270, i64 0
  %t272 = shufflevector <8 x i16> %t271, <8 x i16> poison, <8 x i32> zeroinitializer
  %t273 = ashr <8 x i16> %t272, %t269
  store <8 x i16> %t273, ptr %lv.v1
  br label %block_64
block_60:
  %t274 = load i32, ptr %lv.__i.8
  %t275 = sext i32 %t274 to i64
  %t276 = getelementptr i16, ptr %lv.v1, i64 %t275
  %t277 = load i16, ptr %t276
  %t278 = load i32, ptr %lv.__i.8
  %t279 = sext i32 %t278 to i64
  %t280 = getelementptr i16, ptr %lv.v0, i64 %t279
  %t281 = load i16, ptr %t280
  %t282 = sext i16 %t281 to i32
  %t283 = shl i32 2, %t282
  %t284 = sext i16 %t277 to i32
  %t285 = icmp ne i32 %t284, %t283
  %t286 = zext i1 %t285 to i32
  %t287 = icmp ne i32 %t286, 0
  br i1 %t287, label %block_62, label %block_63
block_61:
  br label %block_58
block_62:
  %t288 = call i32 @abort()
  br label %block_63
block_63:
  br label %for.latch.60
block_64:
  store i32 0, ptr %lv.__i.9
  br label %for.cond.67
for.cond.67:
  %t289 = load i32, ptr %lv.__i.9
  %t290 = icmp slt i32 %t289, 8
  %t291 = zext i1 %t290 to i32
  %t292 = icmp ne i32 %t291, 0
  br i1 %t292, label %block_67, label %block_68
for.latch.67:
  %t293 = load i32, ptr %lv.__i.9
  %t294 = add i32 %t293, 1
  store i32 %t294, ptr %lv.__i.9
  br label %for.cond.67
block_65:
  br label %dowhile.cond.64
dowhile.cond.64:
  %t295 = icmp ne i32 0, 0
  br i1 %t295, label %block_64, label %block_66
block_66:
  %t296 = load <8 x i16>, ptr %lv.v0
  %t297 = trunc i32 2 to i16
  %t298 = insertelement <8 x i16> poison, i16 %t297, i64 0
  %t299 = shufflevector <8 x i16> %t298, <8 x i16> poison, <8 x i32> zeroinitializer
  %t300 = add <8 x i16> %t296, %t299
  store <8 x i16> %t300, ptr %lv.v1
  br label %block_71
block_67:
  %t301 = load i32, ptr %lv.__i.9
  %t302 = sext i32 %t301 to i64
  %t303 = getelementptr i16, ptr %lv.v1, i64 %t302
  %t304 = load i16, ptr %t303
  %t305 = load i32, ptr %lv.__i.9
  %t306 = sext i32 %t305 to i64
  %t307 = getelementptr i16, ptr %lv.v0, i64 %t306
  %t308 = load i16, ptr %t307
  %t309 = sext i16 %t308 to i32
  %t310 = ashr i32 2, %t309
  %t311 = sext i16 %t304 to i32
  %t312 = icmp ne i32 %t311, %t310
  %t313 = zext i1 %t312 to i32
  %t314 = icmp ne i32 %t313, 0
  br i1 %t314, label %block_69, label %block_70
block_68:
  br label %block_65
block_69:
  %t315 = call i32 @abort()
  br label %block_70
block_70:
  br label %for.latch.67
block_71:
  store i32 0, ptr %lv.__i.10
  br label %for.cond.74
for.cond.74:
  %t316 = load i32, ptr %lv.__i.10
  %t317 = icmp slt i32 %t316, 8
  %t318 = zext i1 %t317 to i32
  %t319 = icmp ne i32 %t318, 0
  br i1 %t319, label %block_74, label %block_75
for.latch.74:
  %t320 = load i32, ptr %lv.__i.10
  %t321 = add i32 %t320, 1
  store i32 %t321, ptr %lv.__i.10
  br label %for.cond.74
block_72:
  br label %dowhile.cond.71
dowhile.cond.71:
  %t322 = icmp ne i32 0, 0
  br i1 %t322, label %block_71, label %block_73
block_73:
  %t323 = load <8 x i16>, ptr %lv.v0
  %t324 = trunc i32 2 to i16
  %t325 = insertelement <8 x i16> poison, i16 %t324, i64 0
  %t326 = shufflevector <8 x i16> %t325, <8 x i16> poison, <8 x i32> zeroinitializer
  %t327 = sub <8 x i16> %t323, %t326
  store <8 x i16> %t327, ptr %lv.v1
  br label %block_78
block_74:
  %t328 = load i32, ptr %lv.__i.10
  %t329 = sext i32 %t328 to i64
  %t330 = getelementptr i16, ptr %lv.v1, i64 %t329
  %t331 = load i16, ptr %t330
  %t332 = load i32, ptr %lv.__i.10
  %t333 = sext i32 %t332 to i64
  %t334 = getelementptr i16, ptr %lv.v0, i64 %t333
  %t335 = load i16, ptr %t334
  %t336 = sext i16 %t335 to i32
  %t337 = add i32 %t336, 2
  %t338 = sext i16 %t331 to i32
  %t339 = icmp ne i32 %t338, %t337
  %t340 = zext i1 %t339 to i32
  %t341 = icmp ne i32 %t340, 0
  br i1 %t341, label %block_76, label %block_77
block_75:
  br label %block_72
block_76:
  %t342 = call i32 @abort()
  br label %block_77
block_77:
  br label %for.latch.74
block_78:
  store i32 0, ptr %lv.__i.11
  br label %for.cond.81
for.cond.81:
  %t343 = load i32, ptr %lv.__i.11
  %t344 = icmp slt i32 %t343, 8
  %t345 = zext i1 %t344 to i32
  %t346 = icmp ne i32 %t345, 0
  br i1 %t346, label %block_81, label %block_82
for.latch.81:
  %t347 = load i32, ptr %lv.__i.11
  %t348 = add i32 %t347, 1
  store i32 %t348, ptr %lv.__i.11
  br label %for.cond.81
block_79:
  br label %dowhile.cond.78
dowhile.cond.78:
  %t349 = icmp ne i32 0, 0
  br i1 %t349, label %block_78, label %block_80
block_80:
  %t350 = load <8 x i16>, ptr %lv.v0
  %t351 = trunc i32 2 to i16
  %t352 = insertelement <8 x i16> poison, i16 %t351, i64 0
  %t353 = shufflevector <8 x i16> %t352, <8 x i16> poison, <8 x i32> zeroinitializer
  %t354 = mul <8 x i16> %t350, %t353
  store <8 x i16> %t354, ptr %lv.v1
  br label %block_85
block_81:
  %t355 = load i32, ptr %lv.__i.11
  %t356 = sext i32 %t355 to i64
  %t357 = getelementptr i16, ptr %lv.v1, i64 %t356
  %t358 = load i16, ptr %t357
  %t359 = load i32, ptr %lv.__i.11
  %t360 = sext i32 %t359 to i64
  %t361 = getelementptr i16, ptr %lv.v0, i64 %t360
  %t362 = load i16, ptr %t361
  %t363 = sext i16 %t362 to i32
  %t364 = sub i32 %t363, 2
  %t365 = sext i16 %t358 to i32
  %t366 = icmp ne i32 %t365, %t364
  %t367 = zext i1 %t366 to i32
  %t368 = icmp ne i32 %t367, 0
  br i1 %t368, label %block_83, label %block_84
block_82:
  br label %block_79
block_83:
  %t369 = call i32 @abort()
  br label %block_84
block_84:
  br label %for.latch.81
block_85:
  store i32 0, ptr %lv.__i.12
  br label %for.cond.88
for.cond.88:
  %t370 = load i32, ptr %lv.__i.12
  %t371 = icmp slt i32 %t370, 8
  %t372 = zext i1 %t371 to i32
  %t373 = icmp ne i32 %t372, 0
  br i1 %t373, label %block_88, label %block_89
for.latch.88:
  %t374 = load i32, ptr %lv.__i.12
  %t375 = add i32 %t374, 1
  store i32 %t375, ptr %lv.__i.12
  br label %for.cond.88
block_86:
  br label %dowhile.cond.85
dowhile.cond.85:
  %t376 = icmp ne i32 0, 0
  br i1 %t376, label %block_85, label %block_87
block_87:
  %t377 = load <8 x i16>, ptr %lv.v0
  %t378 = trunc i32 2 to i16
  %t379 = insertelement <8 x i16> poison, i16 %t378, i64 0
  %t380 = shufflevector <8 x i16> %t379, <8 x i16> poison, <8 x i32> zeroinitializer
  %t381 = sdiv <8 x i16> %t377, %t380
  store <8 x i16> %t381, ptr %lv.v1
  br label %block_92
block_88:
  %t382 = load i32, ptr %lv.__i.12
  %t383 = sext i32 %t382 to i64
  %t384 = getelementptr i16, ptr %lv.v1, i64 %t383
  %t385 = load i16, ptr %t384
  %t386 = load i32, ptr %lv.__i.12
  %t387 = sext i32 %t386 to i64
  %t388 = getelementptr i16, ptr %lv.v0, i64 %t387
  %t389 = load i16, ptr %t388
  %t390 = sext i16 %t389 to i32
  %t391 = mul i32 %t390, 2
  %t392 = sext i16 %t385 to i32
  %t393 = icmp ne i32 %t392, %t391
  %t394 = zext i1 %t393 to i32
  %t395 = icmp ne i32 %t394, 0
  br i1 %t395, label %block_90, label %block_91
block_89:
  br label %block_86
block_90:
  %t396 = call i32 @abort()
  br label %block_91
block_91:
  br label %for.latch.88
block_92:
  store i32 0, ptr %lv.__i.13
  br label %for.cond.95
for.cond.95:
  %t397 = load i32, ptr %lv.__i.13
  %t398 = icmp slt i32 %t397, 8
  %t399 = zext i1 %t398 to i32
  %t400 = icmp ne i32 %t399, 0
  br i1 %t400, label %block_95, label %block_96
for.latch.95:
  %t401 = load i32, ptr %lv.__i.13
  %t402 = add i32 %t401, 1
  store i32 %t402, ptr %lv.__i.13
  br label %for.cond.95
block_93:
  br label %dowhile.cond.92
dowhile.cond.92:
  %t403 = icmp ne i32 0, 0
  br i1 %t403, label %block_92, label %block_94
block_94:
  %t404 = load <8 x i16>, ptr %lv.v0
  %t405 = trunc i32 2 to i16
  %t406 = insertelement <8 x i16> poison, i16 %t405, i64 0
  %t407 = shufflevector <8 x i16> %t406, <8 x i16> poison, <8 x i32> zeroinitializer
  %t408 = srem <8 x i16> %t404, %t407
  store <8 x i16> %t408, ptr %lv.v1
  br label %block_99
block_95:
  %t409 = load i32, ptr %lv.__i.13
  %t410 = sext i32 %t409 to i64
  %t411 = getelementptr i16, ptr %lv.v1, i64 %t410
  %t412 = load i16, ptr %t411
  %t413 = load i32, ptr %lv.__i.13
  %t414 = sext i32 %t413 to i64
  %t415 = getelementptr i16, ptr %lv.v0, i64 %t414
  %t416 = load i16, ptr %t415
  %t417 = sext i16 %t416 to i32
  %t418 = sdiv i32 %t417, 2
  %t419 = sext i16 %t412 to i32
  %t420 = icmp ne i32 %t419, %t418
  %t421 = zext i1 %t420 to i32
  %t422 = icmp ne i32 %t421, 0
  br i1 %t422, label %block_97, label %block_98
block_96:
  br label %block_93
block_97:
  %t423 = call i32 @abort()
  br label %block_98
block_98:
  br label %for.latch.95
block_99:
  store i32 0, ptr %lv.__i.14
  br label %for.cond.102
for.cond.102:
  %t424 = load i32, ptr %lv.__i.14
  %t425 = icmp slt i32 %t424, 8
  %t426 = zext i1 %t425 to i32
  %t427 = icmp ne i32 %t426, 0
  br i1 %t427, label %block_102, label %block_103
for.latch.102:
  %t428 = load i32, ptr %lv.__i.14
  %t429 = add i32 %t428, 1
  store i32 %t429, ptr %lv.__i.14
  br label %for.cond.102
block_100:
  br label %dowhile.cond.99
dowhile.cond.99:
  %t430 = icmp ne i32 0, 0
  br i1 %t430, label %block_99, label %block_101
block_101:
  %t431 = load <8 x i16>, ptr %lv.v0
  %t432 = trunc i32 2 to i16
  %t433 = insertelement <8 x i16> poison, i16 %t432, i64 0
  %t434 = shufflevector <8 x i16> %t433, <8 x i16> poison, <8 x i32> zeroinitializer
  %t435 = xor <8 x i16> %t431, %t434
  store <8 x i16> %t435, ptr %lv.v1
  br label %block_106
block_102:
  %t436 = load i32, ptr %lv.__i.14
  %t437 = sext i32 %t436 to i64
  %t438 = getelementptr i16, ptr %lv.v1, i64 %t437
  %t439 = load i16, ptr %t438
  %t440 = load i32, ptr %lv.__i.14
  %t441 = sext i32 %t440 to i64
  %t442 = getelementptr i16, ptr %lv.v0, i64 %t441
  %t443 = load i16, ptr %t442
  %t444 = sext i16 %t443 to i32
  %t445 = srem i32 %t444, 2
  %t446 = sext i16 %t439 to i32
  %t447 = icmp ne i32 %t446, %t445
  %t448 = zext i1 %t447 to i32
  %t449 = icmp ne i32 %t448, 0
  br i1 %t449, label %block_104, label %block_105
block_103:
  br label %block_100
block_104:
  %t450 = call i32 @abort()
  br label %block_105
block_105:
  br label %for.latch.102
block_106:
  store i32 0, ptr %lv.__i.15
  br label %for.cond.109
for.cond.109:
  %t451 = load i32, ptr %lv.__i.15
  %t452 = icmp slt i32 %t451, 8
  %t453 = zext i1 %t452 to i32
  %t454 = icmp ne i32 %t453, 0
  br i1 %t454, label %block_109, label %block_110
for.latch.109:
  %t455 = load i32, ptr %lv.__i.15
  %t456 = add i32 %t455, 1
  store i32 %t456, ptr %lv.__i.15
  br label %for.cond.109
block_107:
  br label %dowhile.cond.106
dowhile.cond.106:
  %t457 = icmp ne i32 0, 0
  br i1 %t457, label %block_106, label %block_108
block_108:
  %t458 = load <8 x i16>, ptr %lv.v0
  %t459 = trunc i32 2 to i16
  %t460 = insertelement <8 x i16> poison, i16 %t459, i64 0
  %t461 = shufflevector <8 x i16> %t460, <8 x i16> poison, <8 x i32> zeroinitializer
  %t462 = and <8 x i16> %t458, %t461
  store <8 x i16> %t462, ptr %lv.v1
  br label %block_113
block_109:
  %t463 = load i32, ptr %lv.__i.15
  %t464 = sext i32 %t463 to i64
  %t465 = getelementptr i16, ptr %lv.v1, i64 %t464
  %t466 = load i16, ptr %t465
  %t467 = load i32, ptr %lv.__i.15
  %t468 = sext i32 %t467 to i64
  %t469 = getelementptr i16, ptr %lv.v0, i64 %t468
  %t470 = load i16, ptr %t469
  %t471 = sext i16 %t470 to i32
  %t472 = xor i32 %t471, 2
  %t473 = sext i16 %t466 to i32
  %t474 = icmp ne i32 %t473, %t472
  %t475 = zext i1 %t474 to i32
  %t476 = icmp ne i32 %t475, 0
  br i1 %t476, label %block_111, label %block_112
block_110:
  br label %block_107
block_111:
  %t477 = call i32 @abort()
  br label %block_112
block_112:
  br label %for.latch.109
block_113:
  store i32 0, ptr %lv.__i.16
  br label %for.cond.116
for.cond.116:
  %t478 = load i32, ptr %lv.__i.16
  %t479 = icmp slt i32 %t478, 8
  %t480 = zext i1 %t479 to i32
  %t481 = icmp ne i32 %t480, 0
  br i1 %t481, label %block_116, label %block_117
for.latch.116:
  %t482 = load i32, ptr %lv.__i.16
  %t483 = add i32 %t482, 1
  store i32 %t483, ptr %lv.__i.16
  br label %for.cond.116
block_114:
  br label %dowhile.cond.113
dowhile.cond.113:
  %t484 = icmp ne i32 0, 0
  br i1 %t484, label %block_113, label %block_115
block_115:
  %t485 = load <8 x i16>, ptr %lv.v0
  %t486 = trunc i32 2 to i16
  %t487 = insertelement <8 x i16> poison, i16 %t486, i64 0
  %t488 = shufflevector <8 x i16> %t487, <8 x i16> poison, <8 x i32> zeroinitializer
  %t489 = or <8 x i16> %t485, %t488
  store <8 x i16> %t489, ptr %lv.v1
  br label %block_120
block_116:
  %t490 = load i32, ptr %lv.__i.16
  %t491 = sext i32 %t490 to i64
  %t492 = getelementptr i16, ptr %lv.v1, i64 %t491
  %t493 = load i16, ptr %t492
  %t494 = load i32, ptr %lv.__i.16
  %t495 = sext i32 %t494 to i64
  %t496 = getelementptr i16, ptr %lv.v0, i64 %t495
  %t497 = load i16, ptr %t496
  %t498 = sext i16 %t497 to i32
  %t499 = and i32 %t498, 2
  %t500 = sext i16 %t493 to i32
  %t501 = icmp ne i32 %t500, %t499
  %t502 = zext i1 %t501 to i32
  %t503 = icmp ne i32 %t502, 0
  br i1 %t503, label %block_118, label %block_119
block_117:
  br label %block_114
block_118:
  %t504 = call i32 @abort()
  br label %block_119
block_119:
  br label %for.latch.116
block_120:
  store i32 0, ptr %lv.__i.17
  br label %for.cond.123
for.cond.123:
  %t505 = load i32, ptr %lv.__i.17
  %t506 = icmp slt i32 %t505, 8
  %t507 = zext i1 %t506 to i32
  %t508 = icmp ne i32 %t507, 0
  br i1 %t508, label %block_123, label %block_124
for.latch.123:
  %t509 = load i32, ptr %lv.__i.17
  %t510 = add i32 %t509, 1
  store i32 %t510, ptr %lv.__i.17
  br label %for.cond.123
block_121:
  br label %dowhile.cond.120
dowhile.cond.120:
  %t511 = icmp ne i32 0, 0
  br i1 %t511, label %block_120, label %block_122
block_122:
  %t512 = load <4 x float>, ptr %lv.f0
  %t513 = fptrunc double 0x4000000000000000 to float
  %t514 = insertelement <4 x float> poison, float %t513, i64 0
  %t515 = shufflevector <4 x float> %t514, <4 x float> poison, <4 x i32> zeroinitializer
  %t516 = fadd <4 x float> %t515, %t512
  store <4 x float> %t516, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_
  %t517 = sext i32 0 to i64
  %t518 = getelementptr float, ptr %lv._clit_, i64 %t517
  %t519 = fptrunc double 0x4000000000000000 to float
  store float %t519, ptr %t518
  %t520 = sext i32 1 to i64
  %t521 = getelementptr float, ptr %lv._clit_, i64 %t520
  %t522 = fptrunc double 0x4000000000000000 to float
  store float %t522, ptr %t521
  %t523 = sext i32 2 to i64
  %t524 = getelementptr float, ptr %lv._clit_, i64 %t523
  %t525 = fptrunc double 0x4000000000000000 to float
  store float %t525, ptr %t524
  %t526 = sext i32 3 to i64
  %t527 = getelementptr float, ptr %lv._clit_, i64 %t526
  %t528 = fptrunc double 0x4000000000000000 to float
  store float %t528, ptr %t527
  %t529 = load <4 x float>, ptr %lv._clit_
  %t530 = load <4 x float>, ptr %lv.f0
  %t531 = fadd <4 x float> %t529, %t530
  store <4 x float> %t531, ptr %lv.f2
  br label %block_127
block_123:
  %t532 = load i32, ptr %lv.__i.17
  %t533 = sext i32 %t532 to i64
  %t534 = getelementptr i16, ptr %lv.v1, i64 %t533
  %t535 = load i16, ptr %t534
  %t536 = load i32, ptr %lv.__i.17
  %t537 = sext i32 %t536 to i64
  %t538 = getelementptr i16, ptr %lv.v0, i64 %t537
  %t539 = load i16, ptr %t538
  %t540 = sext i16 %t539 to i32
  %t541 = or i32 %t540, 2
  %t542 = sext i16 %t535 to i32
  %t543 = icmp ne i32 %t542, %t541
  %t544 = zext i1 %t543 to i32
  %t545 = icmp ne i32 %t544, 0
  br i1 %t545, label %block_125, label %block_126
block_124:
  br label %block_121
block_125:
  %t546 = call i32 @abort()
  br label %block_126
block_126:
  br label %for.latch.123
block_127:
  store i32 0, ptr %lv.__i.18
  br label %for.cond.130
for.cond.130:
  %t547 = load i32, ptr %lv.__i.18
  %t548 = icmp slt i32 %t547, 4
  %t549 = zext i1 %t548 to i32
  %t550 = icmp ne i32 %t549, 0
  br i1 %t550, label %block_130, label %block_131
for.latch.130:
  %t551 = load i32, ptr %lv.__i.18
  %t552 = add i32 %t551, 1
  store i32 %t552, ptr %lv.__i.18
  br label %for.cond.130
block_128:
  br label %dowhile.cond.127
dowhile.cond.127:
  %t553 = icmp ne i32 0, 0
  br i1 %t553, label %block_127, label %block_129
block_129:
  %t554 = load <4 x float>, ptr %lv.f0
  %t555 = fptrunc double 0x4000000000000000 to float
  %t556 = insertelement <4 x float> poison, float %t555, i64 0
  %t557 = shufflevector <4 x float> %t556, <4 x float> poison, <4 x i32> zeroinitializer
  %t558 = fsub <4 x float> %t557, %t554
  store <4 x float> %t558, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.1
  %t559 = sext i32 0 to i64
  %t560 = getelementptr float, ptr %lv._clit_.1, i64 %t559
  %t561 = fptrunc double 0x4000000000000000 to float
  store float %t561, ptr %t560
  %t562 = sext i32 1 to i64
  %t563 = getelementptr float, ptr %lv._clit_.1, i64 %t562
  %t564 = fptrunc double 0x4000000000000000 to float
  store float %t564, ptr %t563
  %t565 = sext i32 2 to i64
  %t566 = getelementptr float, ptr %lv._clit_.1, i64 %t565
  %t567 = fptrunc double 0x4000000000000000 to float
  store float %t567, ptr %t566
  %t568 = sext i32 3 to i64
  %t569 = getelementptr float, ptr %lv._clit_.1, i64 %t568
  %t570 = fptrunc double 0x4000000000000000 to float
  store float %t570, ptr %t569
  %t571 = load <4 x float>, ptr %lv._clit_.1
  %t572 = load <4 x float>, ptr %lv.f0
  %t573 = fsub <4 x float> %t571, %t572
  store <4 x float> %t573, ptr %lv.f2
  br label %block_134
block_130:
  %t574 = load i32, ptr %lv.__i.18
  %t575 = sext i32 %t574 to i64
  %t576 = getelementptr float, ptr %lv.f1, i64 %t575
  %t577 = load float, ptr %t576
  %t578 = load i32, ptr %lv.__i.18
  %t579 = sext i32 %t578 to i64
  %t580 = getelementptr float, ptr %lv.f2, i64 %t579
  %t581 = load float, ptr %t580
  %t582 = fcmp une float %t577, %t581
  %t583 = zext i1 %t582 to i32
  %t584 = icmp ne i32 %t583, 0
  br i1 %t584, label %block_132, label %block_133
block_131:
  br label %block_128
block_132:
  %t585 = call i32 @abort()
  br label %block_133
block_133:
  br label %for.latch.130
block_134:
  store i32 0, ptr %lv.__i.19
  br label %for.cond.137
for.cond.137:
  %t586 = load i32, ptr %lv.__i.19
  %t587 = icmp slt i32 %t586, 4
  %t588 = zext i1 %t587 to i32
  %t589 = icmp ne i32 %t588, 0
  br i1 %t589, label %block_137, label %block_138
for.latch.137:
  %t590 = load i32, ptr %lv.__i.19
  %t591 = add i32 %t590, 1
  store i32 %t591, ptr %lv.__i.19
  br label %for.cond.137
block_135:
  br label %dowhile.cond.134
dowhile.cond.134:
  %t592 = icmp ne i32 0, 0
  br i1 %t592, label %block_134, label %block_136
block_136:
  %t593 = load <4 x float>, ptr %lv.f0
  %t594 = fptrunc double 0x4000000000000000 to float
  %t595 = insertelement <4 x float> poison, float %t594, i64 0
  %t596 = shufflevector <4 x float> %t595, <4 x float> poison, <4 x i32> zeroinitializer
  %t597 = fmul <4 x float> %t596, %t593
  store <4 x float> %t597, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.2
  %t598 = sext i32 0 to i64
  %t599 = getelementptr float, ptr %lv._clit_.2, i64 %t598
  %t600 = fptrunc double 0x4000000000000000 to float
  store float %t600, ptr %t599
  %t601 = sext i32 1 to i64
  %t602 = getelementptr float, ptr %lv._clit_.2, i64 %t601
  %t603 = fptrunc double 0x4000000000000000 to float
  store float %t603, ptr %t602
  %t604 = sext i32 2 to i64
  %t605 = getelementptr float, ptr %lv._clit_.2, i64 %t604
  %t606 = fptrunc double 0x4000000000000000 to float
  store float %t606, ptr %t605
  %t607 = sext i32 3 to i64
  %t608 = getelementptr float, ptr %lv._clit_.2, i64 %t607
  %t609 = fptrunc double 0x4000000000000000 to float
  store float %t609, ptr %t608
  %t610 = load <4 x float>, ptr %lv._clit_.2
  %t611 = load <4 x float>, ptr %lv.f0
  %t612 = fmul <4 x float> %t610, %t611
  store <4 x float> %t612, ptr %lv.f2
  br label %block_141
block_137:
  %t613 = load i32, ptr %lv.__i.19
  %t614 = sext i32 %t613 to i64
  %t615 = getelementptr float, ptr %lv.f1, i64 %t614
  %t616 = load float, ptr %t615
  %t617 = load i32, ptr %lv.__i.19
  %t618 = sext i32 %t617 to i64
  %t619 = getelementptr float, ptr %lv.f2, i64 %t618
  %t620 = load float, ptr %t619
  %t621 = fcmp une float %t616, %t620
  %t622 = zext i1 %t621 to i32
  %t623 = icmp ne i32 %t622, 0
  br i1 %t623, label %block_139, label %block_140
block_138:
  br label %block_135
block_139:
  %t624 = call i32 @abort()
  br label %block_140
block_140:
  br label %for.latch.137
block_141:
  store i32 0, ptr %lv.__i.20
  br label %for.cond.144
for.cond.144:
  %t625 = load i32, ptr %lv.__i.20
  %t626 = icmp slt i32 %t625, 4
  %t627 = zext i1 %t626 to i32
  %t628 = icmp ne i32 %t627, 0
  br i1 %t628, label %block_144, label %block_145
for.latch.144:
  %t629 = load i32, ptr %lv.__i.20
  %t630 = add i32 %t629, 1
  store i32 %t630, ptr %lv.__i.20
  br label %for.cond.144
block_142:
  br label %dowhile.cond.141
dowhile.cond.141:
  %t631 = icmp ne i32 0, 0
  br i1 %t631, label %block_141, label %block_143
block_143:
  %t632 = load <4 x float>, ptr %lv.f0
  %t633 = fptrunc double 0x4000000000000000 to float
  %t634 = insertelement <4 x float> poison, float %t633, i64 0
  %t635 = shufflevector <4 x float> %t634, <4 x float> poison, <4 x i32> zeroinitializer
  %t636 = fdiv <4 x float> %t635, %t632
  store <4 x float> %t636, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.3
  %t637 = sext i32 0 to i64
  %t638 = getelementptr float, ptr %lv._clit_.3, i64 %t637
  %t639 = fptrunc double 0x4000000000000000 to float
  store float %t639, ptr %t638
  %t640 = sext i32 1 to i64
  %t641 = getelementptr float, ptr %lv._clit_.3, i64 %t640
  %t642 = fptrunc double 0x4000000000000000 to float
  store float %t642, ptr %t641
  %t643 = sext i32 2 to i64
  %t644 = getelementptr float, ptr %lv._clit_.3, i64 %t643
  %t645 = fptrunc double 0x4000000000000000 to float
  store float %t645, ptr %t644
  %t646 = sext i32 3 to i64
  %t647 = getelementptr float, ptr %lv._clit_.3, i64 %t646
  %t648 = fptrunc double 0x4000000000000000 to float
  store float %t648, ptr %t647
  %t649 = load <4 x float>, ptr %lv._clit_.3
  %t650 = load <4 x float>, ptr %lv.f0
  %t651 = fdiv <4 x float> %t649, %t650
  store <4 x float> %t651, ptr %lv.f2
  br label %block_148
block_144:
  %t652 = load i32, ptr %lv.__i.20
  %t653 = sext i32 %t652 to i64
  %t654 = getelementptr float, ptr %lv.f1, i64 %t653
  %t655 = load float, ptr %t654
  %t656 = load i32, ptr %lv.__i.20
  %t657 = sext i32 %t656 to i64
  %t658 = getelementptr float, ptr %lv.f2, i64 %t657
  %t659 = load float, ptr %t658
  %t660 = fcmp une float %t655, %t659
  %t661 = zext i1 %t660 to i32
  %t662 = icmp ne i32 %t661, 0
  br i1 %t662, label %block_146, label %block_147
block_145:
  br label %block_142
block_146:
  %t663 = call i32 @abort()
  br label %block_147
block_147:
  br label %for.latch.144
block_148:
  store i32 0, ptr %lv.__i.21
  br label %for.cond.151
for.cond.151:
  %t664 = load i32, ptr %lv.__i.21
  %t665 = icmp slt i32 %t664, 4
  %t666 = zext i1 %t665 to i32
  %t667 = icmp ne i32 %t666, 0
  br i1 %t667, label %block_151, label %block_152
for.latch.151:
  %t668 = load i32, ptr %lv.__i.21
  %t669 = add i32 %t668, 1
  store i32 %t669, ptr %lv.__i.21
  br label %for.cond.151
block_149:
  br label %dowhile.cond.148
dowhile.cond.148:
  %t670 = icmp ne i32 0, 0
  br i1 %t670, label %block_148, label %block_150
block_150:
  %t671 = load <4 x float>, ptr %lv.f0
  %t672 = fptrunc double 0x4000000000000000 to float
  %t673 = insertelement <4 x float> poison, float %t672, i64 0
  %t674 = shufflevector <4 x float> %t673, <4 x float> poison, <4 x i32> zeroinitializer
  %t675 = fadd <4 x float> %t671, %t674
  store <4 x float> %t675, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.4
  %t676 = sext i32 0 to i64
  %t677 = getelementptr float, ptr %lv._clit_.4, i64 %t676
  %t678 = fptrunc double 0x4000000000000000 to float
  store float %t678, ptr %t677
  %t679 = sext i32 1 to i64
  %t680 = getelementptr float, ptr %lv._clit_.4, i64 %t679
  %t681 = fptrunc double 0x4000000000000000 to float
  store float %t681, ptr %t680
  %t682 = sext i32 2 to i64
  %t683 = getelementptr float, ptr %lv._clit_.4, i64 %t682
  %t684 = fptrunc double 0x4000000000000000 to float
  store float %t684, ptr %t683
  %t685 = sext i32 3 to i64
  %t686 = getelementptr float, ptr %lv._clit_.4, i64 %t685
  %t687 = fptrunc double 0x4000000000000000 to float
  store float %t687, ptr %t686
  %t688 = load <4 x float>, ptr %lv.f0
  %t689 = load <4 x float>, ptr %lv._clit_.4
  %t690 = fadd <4 x float> %t688, %t689
  store <4 x float> %t690, ptr %lv.f2
  br label %block_155
block_151:
  %t691 = load i32, ptr %lv.__i.21
  %t692 = sext i32 %t691 to i64
  %t693 = getelementptr float, ptr %lv.f1, i64 %t692
  %t694 = load float, ptr %t693
  %t695 = load i32, ptr %lv.__i.21
  %t696 = sext i32 %t695 to i64
  %t697 = getelementptr float, ptr %lv.f2, i64 %t696
  %t698 = load float, ptr %t697
  %t699 = fcmp une float %t694, %t698
  %t700 = zext i1 %t699 to i32
  %t701 = icmp ne i32 %t700, 0
  br i1 %t701, label %block_153, label %block_154
block_152:
  br label %block_149
block_153:
  %t702 = call i32 @abort()
  br label %block_154
block_154:
  br label %for.latch.151
block_155:
  store i32 0, ptr %lv.__i.22
  br label %for.cond.158
for.cond.158:
  %t703 = load i32, ptr %lv.__i.22
  %t704 = icmp slt i32 %t703, 4
  %t705 = zext i1 %t704 to i32
  %t706 = icmp ne i32 %t705, 0
  br i1 %t706, label %block_158, label %block_159
for.latch.158:
  %t707 = load i32, ptr %lv.__i.22
  %t708 = add i32 %t707, 1
  store i32 %t708, ptr %lv.__i.22
  br label %for.cond.158
block_156:
  br label %dowhile.cond.155
dowhile.cond.155:
  %t709 = icmp ne i32 0, 0
  br i1 %t709, label %block_155, label %block_157
block_157:
  %t710 = load <4 x float>, ptr %lv.f0
  %t711 = fptrunc double 0x4000000000000000 to float
  %t712 = insertelement <4 x float> poison, float %t711, i64 0
  %t713 = shufflevector <4 x float> %t712, <4 x float> poison, <4 x i32> zeroinitializer
  %t714 = fsub <4 x float> %t710, %t713
  store <4 x float> %t714, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.5
  %t715 = sext i32 0 to i64
  %t716 = getelementptr float, ptr %lv._clit_.5, i64 %t715
  %t717 = fptrunc double 0x4000000000000000 to float
  store float %t717, ptr %t716
  %t718 = sext i32 1 to i64
  %t719 = getelementptr float, ptr %lv._clit_.5, i64 %t718
  %t720 = fptrunc double 0x4000000000000000 to float
  store float %t720, ptr %t719
  %t721 = sext i32 2 to i64
  %t722 = getelementptr float, ptr %lv._clit_.5, i64 %t721
  %t723 = fptrunc double 0x4000000000000000 to float
  store float %t723, ptr %t722
  %t724 = sext i32 3 to i64
  %t725 = getelementptr float, ptr %lv._clit_.5, i64 %t724
  %t726 = fptrunc double 0x4000000000000000 to float
  store float %t726, ptr %t725
  %t727 = load <4 x float>, ptr %lv.f0
  %t728 = load <4 x float>, ptr %lv._clit_.5
  %t729 = fsub <4 x float> %t727, %t728
  store <4 x float> %t729, ptr %lv.f2
  br label %block_162
block_158:
  %t730 = load i32, ptr %lv.__i.22
  %t731 = sext i32 %t730 to i64
  %t732 = getelementptr float, ptr %lv.f1, i64 %t731
  %t733 = load float, ptr %t732
  %t734 = load i32, ptr %lv.__i.22
  %t735 = sext i32 %t734 to i64
  %t736 = getelementptr float, ptr %lv.f2, i64 %t735
  %t737 = load float, ptr %t736
  %t738 = fcmp une float %t733, %t737
  %t739 = zext i1 %t738 to i32
  %t740 = icmp ne i32 %t739, 0
  br i1 %t740, label %block_160, label %block_161
block_159:
  br label %block_156
block_160:
  %t741 = call i32 @abort()
  br label %block_161
block_161:
  br label %for.latch.158
block_162:
  store i32 0, ptr %lv.__i.23
  br label %for.cond.165
for.cond.165:
  %t742 = load i32, ptr %lv.__i.23
  %t743 = icmp slt i32 %t742, 4
  %t744 = zext i1 %t743 to i32
  %t745 = icmp ne i32 %t744, 0
  br i1 %t745, label %block_165, label %block_166
for.latch.165:
  %t746 = load i32, ptr %lv.__i.23
  %t747 = add i32 %t746, 1
  store i32 %t747, ptr %lv.__i.23
  br label %for.cond.165
block_163:
  br label %dowhile.cond.162
dowhile.cond.162:
  %t748 = icmp ne i32 0, 0
  br i1 %t748, label %block_162, label %block_164
block_164:
  %t749 = load <4 x float>, ptr %lv.f0
  %t750 = fptrunc double 0x4000000000000000 to float
  %t751 = insertelement <4 x float> poison, float %t750, i64 0
  %t752 = shufflevector <4 x float> %t751, <4 x float> poison, <4 x i32> zeroinitializer
  %t753 = fmul <4 x float> %t749, %t752
  store <4 x float> %t753, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.6
  %t754 = sext i32 0 to i64
  %t755 = getelementptr float, ptr %lv._clit_.6, i64 %t754
  %t756 = fptrunc double 0x4000000000000000 to float
  store float %t756, ptr %t755
  %t757 = sext i32 1 to i64
  %t758 = getelementptr float, ptr %lv._clit_.6, i64 %t757
  %t759 = fptrunc double 0x4000000000000000 to float
  store float %t759, ptr %t758
  %t760 = sext i32 2 to i64
  %t761 = getelementptr float, ptr %lv._clit_.6, i64 %t760
  %t762 = fptrunc double 0x4000000000000000 to float
  store float %t762, ptr %t761
  %t763 = sext i32 3 to i64
  %t764 = getelementptr float, ptr %lv._clit_.6, i64 %t763
  %t765 = fptrunc double 0x4000000000000000 to float
  store float %t765, ptr %t764
  %t766 = load <4 x float>, ptr %lv.f0
  %t767 = load <4 x float>, ptr %lv._clit_.6
  %t768 = fmul <4 x float> %t766, %t767
  store <4 x float> %t768, ptr %lv.f2
  br label %block_169
block_165:
  %t769 = load i32, ptr %lv.__i.23
  %t770 = sext i32 %t769 to i64
  %t771 = getelementptr float, ptr %lv.f1, i64 %t770
  %t772 = load float, ptr %t771
  %t773 = load i32, ptr %lv.__i.23
  %t774 = sext i32 %t773 to i64
  %t775 = getelementptr float, ptr %lv.f2, i64 %t774
  %t776 = load float, ptr %t775
  %t777 = fcmp une float %t772, %t776
  %t778 = zext i1 %t777 to i32
  %t779 = icmp ne i32 %t778, 0
  br i1 %t779, label %block_167, label %block_168
block_166:
  br label %block_163
block_167:
  %t780 = call i32 @abort()
  br label %block_168
block_168:
  br label %for.latch.165
block_169:
  store i32 0, ptr %lv.__i.24
  br label %for.cond.172
for.cond.172:
  %t781 = load i32, ptr %lv.__i.24
  %t782 = icmp slt i32 %t781, 4
  %t783 = zext i1 %t782 to i32
  %t784 = icmp ne i32 %t783, 0
  br i1 %t784, label %block_172, label %block_173
for.latch.172:
  %t785 = load i32, ptr %lv.__i.24
  %t786 = add i32 %t785, 1
  store i32 %t786, ptr %lv.__i.24
  br label %for.cond.172
block_170:
  br label %dowhile.cond.169
dowhile.cond.169:
  %t787 = icmp ne i32 0, 0
  br i1 %t787, label %block_169, label %block_171
block_171:
  %t788 = load <4 x float>, ptr %lv.f0
  %t789 = fptrunc double 0x4000000000000000 to float
  %t790 = insertelement <4 x float> poison, float %t789, i64 0
  %t791 = shufflevector <4 x float> %t790, <4 x float> poison, <4 x i32> zeroinitializer
  %t792 = fdiv <4 x float> %t788, %t791
  store <4 x float> %t792, ptr %lv.f1
  store <4 x float> zeroinitializer, ptr %lv._clit_.7
  %t793 = sext i32 0 to i64
  %t794 = getelementptr float, ptr %lv._clit_.7, i64 %t793
  %t795 = fptrunc double 0x4000000000000000 to float
  store float %t795, ptr %t794
  %t796 = sext i32 1 to i64
  %t797 = getelementptr float, ptr %lv._clit_.7, i64 %t796
  %t798 = fptrunc double 0x4000000000000000 to float
  store float %t798, ptr %t797
  %t799 = sext i32 2 to i64
  %t800 = getelementptr float, ptr %lv._clit_.7, i64 %t799
  %t801 = fptrunc double 0x4000000000000000 to float
  store float %t801, ptr %t800
  %t802 = sext i32 3 to i64
  %t803 = getelementptr float, ptr %lv._clit_.7, i64 %t802
  %t804 = fptrunc double 0x4000000000000000 to float
  store float %t804, ptr %t803
  %t805 = load <4 x float>, ptr %lv.f0
  %t806 = load <4 x float>, ptr %lv._clit_.7
  %t807 = fdiv <4 x float> %t805, %t806
  store <4 x float> %t807, ptr %lv.f2
  br label %block_176
block_172:
  %t808 = load i32, ptr %lv.__i.24
  %t809 = sext i32 %t808 to i64
  %t810 = getelementptr float, ptr %lv.f1, i64 %t809
  %t811 = load float, ptr %t810
  %t812 = load i32, ptr %lv.__i.24
  %t813 = sext i32 %t812 to i64
  %t814 = getelementptr float, ptr %lv.f2, i64 %t813
  %t815 = load float, ptr %t814
  %t816 = fcmp une float %t811, %t815
  %t817 = zext i1 %t816 to i32
  %t818 = icmp ne i32 %t817, 0
  br i1 %t818, label %block_174, label %block_175
block_173:
  br label %block_170
block_174:
  %t819 = call i32 @abort()
  br label %block_175
block_175:
  br label %for.latch.172
block_176:
  store i32 0, ptr %lv.__i.25
  br label %for.cond.179
for.cond.179:
  %t820 = load i32, ptr %lv.__i.25
  %t821 = icmp slt i32 %t820, 4
  %t822 = zext i1 %t821 to i32
  %t823 = icmp ne i32 %t822, 0
  br i1 %t823, label %block_179, label %block_180
for.latch.179:
  %t824 = load i32, ptr %lv.__i.25
  %t825 = add i32 %t824, 1
  store i32 %t825, ptr %lv.__i.25
  br label %for.cond.179
block_177:
  br label %dowhile.cond.176
dowhile.cond.176:
  %t826 = icmp ne i32 0, 0
  br i1 %t826, label %block_176, label %block_178
block_178:
  %t827 = load <2 x double>, ptr %lv.d0
  %t828 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t829 = shufflevector <2 x double> %t828, <2 x double> poison, <2 x i32> zeroinitializer
  %t830 = fadd <2 x double> %t829, %t827
  store <2 x double> %t830, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.8
  %t831 = sext i32 0 to i64
  %t832 = getelementptr double, ptr %lv._clit_.8, i64 %t831
  store double 0x4000000000000000, ptr %t832
  %t833 = sext i32 1 to i64
  %t834 = getelementptr double, ptr %lv._clit_.8, i64 %t833
  store double 0x4000000000000000, ptr %t834
  %t835 = load <2 x double>, ptr %lv._clit_.8
  %t836 = load <2 x double>, ptr %lv.d0
  %t837 = fadd <2 x double> %t835, %t836
  store <2 x double> %t837, ptr %lv.d2
  br label %block_183
block_179:
  %t838 = load i32, ptr %lv.__i.25
  %t839 = sext i32 %t838 to i64
  %t840 = getelementptr float, ptr %lv.f1, i64 %t839
  %t841 = load float, ptr %t840
  %t842 = load i32, ptr %lv.__i.25
  %t843 = sext i32 %t842 to i64
  %t844 = getelementptr float, ptr %lv.f2, i64 %t843
  %t845 = load float, ptr %t844
  %t846 = fcmp une float %t841, %t845
  %t847 = zext i1 %t846 to i32
  %t848 = icmp ne i32 %t847, 0
  br i1 %t848, label %block_181, label %block_182
block_180:
  br label %block_177
block_181:
  %t849 = call i32 @abort()
  br label %block_182
block_182:
  br label %for.latch.179
block_183:
  store i32 0, ptr %lv.__i.26
  br label %for.cond.186
for.cond.186:
  %t850 = load i32, ptr %lv.__i.26
  %t851 = icmp slt i32 %t850, 2
  %t852 = zext i1 %t851 to i32
  %t853 = icmp ne i32 %t852, 0
  br i1 %t853, label %block_186, label %block_187
for.latch.186:
  %t854 = load i32, ptr %lv.__i.26
  %t855 = add i32 %t854, 1
  store i32 %t855, ptr %lv.__i.26
  br label %for.cond.186
block_184:
  br label %dowhile.cond.183
dowhile.cond.183:
  %t856 = icmp ne i32 0, 0
  br i1 %t856, label %block_183, label %block_185
block_185:
  %t857 = load <2 x double>, ptr %lv.d0
  %t858 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t859 = shufflevector <2 x double> %t858, <2 x double> poison, <2 x i32> zeroinitializer
  %t860 = fsub <2 x double> %t859, %t857
  store <2 x double> %t860, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.9
  %t861 = sext i32 0 to i64
  %t862 = getelementptr double, ptr %lv._clit_.9, i64 %t861
  store double 0x4000000000000000, ptr %t862
  %t863 = sext i32 1 to i64
  %t864 = getelementptr double, ptr %lv._clit_.9, i64 %t863
  store double 0x4000000000000000, ptr %t864
  %t865 = load <2 x double>, ptr %lv._clit_.9
  %t866 = load <2 x double>, ptr %lv.d0
  %t867 = fsub <2 x double> %t865, %t866
  store <2 x double> %t867, ptr %lv.d2
  br label %block_190
block_186:
  %t868 = load i32, ptr %lv.__i.26
  %t869 = sext i32 %t868 to i64
  %t870 = getelementptr double, ptr %lv.d1, i64 %t869
  %t871 = load double, ptr %t870
  %t872 = load i32, ptr %lv.__i.26
  %t873 = sext i32 %t872 to i64
  %t874 = getelementptr double, ptr %lv.d2, i64 %t873
  %t875 = load double, ptr %t874
  %t876 = fcmp une double %t871, %t875
  %t877 = zext i1 %t876 to i32
  %t878 = icmp ne i32 %t877, 0
  br i1 %t878, label %block_188, label %block_189
block_187:
  br label %block_184
block_188:
  %t879 = call i32 @abort()
  br label %block_189
block_189:
  br label %for.latch.186
block_190:
  store i32 0, ptr %lv.__i.27
  br label %for.cond.193
for.cond.193:
  %t880 = load i32, ptr %lv.__i.27
  %t881 = icmp slt i32 %t880, 2
  %t882 = zext i1 %t881 to i32
  %t883 = icmp ne i32 %t882, 0
  br i1 %t883, label %block_193, label %block_194
for.latch.193:
  %t884 = load i32, ptr %lv.__i.27
  %t885 = add i32 %t884, 1
  store i32 %t885, ptr %lv.__i.27
  br label %for.cond.193
block_191:
  br label %dowhile.cond.190
dowhile.cond.190:
  %t886 = icmp ne i32 0, 0
  br i1 %t886, label %block_190, label %block_192
block_192:
  %t887 = load <2 x double>, ptr %lv.d0
  %t888 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t889 = shufflevector <2 x double> %t888, <2 x double> poison, <2 x i32> zeroinitializer
  %t890 = fmul <2 x double> %t889, %t887
  store <2 x double> %t890, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.10
  %t891 = sext i32 0 to i64
  %t892 = getelementptr double, ptr %lv._clit_.10, i64 %t891
  store double 0x4000000000000000, ptr %t892
  %t893 = sext i32 1 to i64
  %t894 = getelementptr double, ptr %lv._clit_.10, i64 %t893
  store double 0x4000000000000000, ptr %t894
  %t895 = load <2 x double>, ptr %lv._clit_.10
  %t896 = load <2 x double>, ptr %lv.d0
  %t897 = fmul <2 x double> %t895, %t896
  store <2 x double> %t897, ptr %lv.d2
  br label %block_197
block_193:
  %t898 = load i32, ptr %lv.__i.27
  %t899 = sext i32 %t898 to i64
  %t900 = getelementptr double, ptr %lv.d1, i64 %t899
  %t901 = load double, ptr %t900
  %t902 = load i32, ptr %lv.__i.27
  %t903 = sext i32 %t902 to i64
  %t904 = getelementptr double, ptr %lv.d2, i64 %t903
  %t905 = load double, ptr %t904
  %t906 = fcmp une double %t901, %t905
  %t907 = zext i1 %t906 to i32
  %t908 = icmp ne i32 %t907, 0
  br i1 %t908, label %block_195, label %block_196
block_194:
  br label %block_191
block_195:
  %t909 = call i32 @abort()
  br label %block_196
block_196:
  br label %for.latch.193
block_197:
  store i32 0, ptr %lv.__i.28
  br label %for.cond.200
for.cond.200:
  %t910 = load i32, ptr %lv.__i.28
  %t911 = icmp slt i32 %t910, 2
  %t912 = zext i1 %t911 to i32
  %t913 = icmp ne i32 %t912, 0
  br i1 %t913, label %block_200, label %block_201
for.latch.200:
  %t914 = load i32, ptr %lv.__i.28
  %t915 = add i32 %t914, 1
  store i32 %t915, ptr %lv.__i.28
  br label %for.cond.200
block_198:
  br label %dowhile.cond.197
dowhile.cond.197:
  %t916 = icmp ne i32 0, 0
  br i1 %t916, label %block_197, label %block_199
block_199:
  %t917 = load <2 x double>, ptr %lv.d0
  %t918 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t919 = shufflevector <2 x double> %t918, <2 x double> poison, <2 x i32> zeroinitializer
  %t920 = fdiv <2 x double> %t919, %t917
  store <2 x double> %t920, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.11
  %t921 = sext i32 0 to i64
  %t922 = getelementptr double, ptr %lv._clit_.11, i64 %t921
  store double 0x4000000000000000, ptr %t922
  %t923 = sext i32 1 to i64
  %t924 = getelementptr double, ptr %lv._clit_.11, i64 %t923
  store double 0x4000000000000000, ptr %t924
  %t925 = load <2 x double>, ptr %lv._clit_.11
  %t926 = load <2 x double>, ptr %lv.d0
  %t927 = fdiv <2 x double> %t925, %t926
  store <2 x double> %t927, ptr %lv.d2
  br label %block_204
block_200:
  %t928 = load i32, ptr %lv.__i.28
  %t929 = sext i32 %t928 to i64
  %t930 = getelementptr double, ptr %lv.d1, i64 %t929
  %t931 = load double, ptr %t930
  %t932 = load i32, ptr %lv.__i.28
  %t933 = sext i32 %t932 to i64
  %t934 = getelementptr double, ptr %lv.d2, i64 %t933
  %t935 = load double, ptr %t934
  %t936 = fcmp une double %t931, %t935
  %t937 = zext i1 %t936 to i32
  %t938 = icmp ne i32 %t937, 0
  br i1 %t938, label %block_202, label %block_203
block_201:
  br label %block_198
block_202:
  %t939 = call i32 @abort()
  br label %block_203
block_203:
  br label %for.latch.200
block_204:
  store i32 0, ptr %lv.__i.29
  br label %for.cond.207
for.cond.207:
  %t940 = load i32, ptr %lv.__i.29
  %t941 = icmp slt i32 %t940, 2
  %t942 = zext i1 %t941 to i32
  %t943 = icmp ne i32 %t942, 0
  br i1 %t943, label %block_207, label %block_208
for.latch.207:
  %t944 = load i32, ptr %lv.__i.29
  %t945 = add i32 %t944, 1
  store i32 %t945, ptr %lv.__i.29
  br label %for.cond.207
block_205:
  br label %dowhile.cond.204
dowhile.cond.204:
  %t946 = icmp ne i32 0, 0
  br i1 %t946, label %block_204, label %block_206
block_206:
  %t947 = load <2 x double>, ptr %lv.d0
  %t948 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t949 = shufflevector <2 x double> %t948, <2 x double> poison, <2 x i32> zeroinitializer
  %t950 = fadd <2 x double> %t947, %t949
  store <2 x double> %t950, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.12
  %t951 = sext i32 0 to i64
  %t952 = getelementptr double, ptr %lv._clit_.12, i64 %t951
  store double 0x4000000000000000, ptr %t952
  %t953 = sext i32 1 to i64
  %t954 = getelementptr double, ptr %lv._clit_.12, i64 %t953
  store double 0x4000000000000000, ptr %t954
  %t955 = load <2 x double>, ptr %lv.d0
  %t956 = load <2 x double>, ptr %lv._clit_.12
  %t957 = fadd <2 x double> %t955, %t956
  store <2 x double> %t957, ptr %lv.d2
  br label %block_211
block_207:
  %t958 = load i32, ptr %lv.__i.29
  %t959 = sext i32 %t958 to i64
  %t960 = getelementptr double, ptr %lv.d1, i64 %t959
  %t961 = load double, ptr %t960
  %t962 = load i32, ptr %lv.__i.29
  %t963 = sext i32 %t962 to i64
  %t964 = getelementptr double, ptr %lv.d2, i64 %t963
  %t965 = load double, ptr %t964
  %t966 = fcmp une double %t961, %t965
  %t967 = zext i1 %t966 to i32
  %t968 = icmp ne i32 %t967, 0
  br i1 %t968, label %block_209, label %block_210
block_208:
  br label %block_205
block_209:
  %t969 = call i32 @abort()
  br label %block_210
block_210:
  br label %for.latch.207
block_211:
  store i32 0, ptr %lv.__i.30
  br label %for.cond.214
for.cond.214:
  %t970 = load i32, ptr %lv.__i.30
  %t971 = icmp slt i32 %t970, 2
  %t972 = zext i1 %t971 to i32
  %t973 = icmp ne i32 %t972, 0
  br i1 %t973, label %block_214, label %block_215
for.latch.214:
  %t974 = load i32, ptr %lv.__i.30
  %t975 = add i32 %t974, 1
  store i32 %t975, ptr %lv.__i.30
  br label %for.cond.214
block_212:
  br label %dowhile.cond.211
dowhile.cond.211:
  %t976 = icmp ne i32 0, 0
  br i1 %t976, label %block_211, label %block_213
block_213:
  %t977 = load <2 x double>, ptr %lv.d0
  %t978 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t979 = shufflevector <2 x double> %t978, <2 x double> poison, <2 x i32> zeroinitializer
  %t980 = fsub <2 x double> %t977, %t979
  store <2 x double> %t980, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.13
  %t981 = sext i32 0 to i64
  %t982 = getelementptr double, ptr %lv._clit_.13, i64 %t981
  store double 0x4000000000000000, ptr %t982
  %t983 = sext i32 1 to i64
  %t984 = getelementptr double, ptr %lv._clit_.13, i64 %t983
  store double 0x4000000000000000, ptr %t984
  %t985 = load <2 x double>, ptr %lv.d0
  %t986 = load <2 x double>, ptr %lv._clit_.13
  %t987 = fsub <2 x double> %t985, %t986
  store <2 x double> %t987, ptr %lv.d2
  br label %block_218
block_214:
  %t988 = load i32, ptr %lv.__i.30
  %t989 = sext i32 %t988 to i64
  %t990 = getelementptr double, ptr %lv.d1, i64 %t989
  %t991 = load double, ptr %t990
  %t992 = load i32, ptr %lv.__i.30
  %t993 = sext i32 %t992 to i64
  %t994 = getelementptr double, ptr %lv.d2, i64 %t993
  %t995 = load double, ptr %t994
  %t996 = fcmp une double %t991, %t995
  %t997 = zext i1 %t996 to i32
  %t998 = icmp ne i32 %t997, 0
  br i1 %t998, label %block_216, label %block_217
block_215:
  br label %block_212
block_216:
  %t999 = call i32 @abort()
  br label %block_217
block_217:
  br label %for.latch.214
block_218:
  store i32 0, ptr %lv.__i.31
  br label %for.cond.221
for.cond.221:
  %t1000 = load i32, ptr %lv.__i.31
  %t1001 = icmp slt i32 %t1000, 2
  %t1002 = zext i1 %t1001 to i32
  %t1003 = icmp ne i32 %t1002, 0
  br i1 %t1003, label %block_221, label %block_222
for.latch.221:
  %t1004 = load i32, ptr %lv.__i.31
  %t1005 = add i32 %t1004, 1
  store i32 %t1005, ptr %lv.__i.31
  br label %for.cond.221
block_219:
  br label %dowhile.cond.218
dowhile.cond.218:
  %t1006 = icmp ne i32 0, 0
  br i1 %t1006, label %block_218, label %block_220
block_220:
  %t1007 = load <2 x double>, ptr %lv.d0
  %t1008 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t1009 = shufflevector <2 x double> %t1008, <2 x double> poison, <2 x i32> zeroinitializer
  %t1010 = fmul <2 x double> %t1007, %t1009
  store <2 x double> %t1010, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.14
  %t1011 = sext i32 0 to i64
  %t1012 = getelementptr double, ptr %lv._clit_.14, i64 %t1011
  store double 0x4000000000000000, ptr %t1012
  %t1013 = sext i32 1 to i64
  %t1014 = getelementptr double, ptr %lv._clit_.14, i64 %t1013
  store double 0x4000000000000000, ptr %t1014
  %t1015 = load <2 x double>, ptr %lv.d0
  %t1016 = load <2 x double>, ptr %lv._clit_.14
  %t1017 = fmul <2 x double> %t1015, %t1016
  store <2 x double> %t1017, ptr %lv.d2
  br label %block_225
block_221:
  %t1018 = load i32, ptr %lv.__i.31
  %t1019 = sext i32 %t1018 to i64
  %t1020 = getelementptr double, ptr %lv.d1, i64 %t1019
  %t1021 = load double, ptr %t1020
  %t1022 = load i32, ptr %lv.__i.31
  %t1023 = sext i32 %t1022 to i64
  %t1024 = getelementptr double, ptr %lv.d2, i64 %t1023
  %t1025 = load double, ptr %t1024
  %t1026 = fcmp une double %t1021, %t1025
  %t1027 = zext i1 %t1026 to i32
  %t1028 = icmp ne i32 %t1027, 0
  br i1 %t1028, label %block_223, label %block_224
block_222:
  br label %block_219
block_223:
  %t1029 = call i32 @abort()
  br label %block_224
block_224:
  br label %for.latch.221
block_225:
  store i32 0, ptr %lv.__i.32
  br label %for.cond.228
for.cond.228:
  %t1030 = load i32, ptr %lv.__i.32
  %t1031 = icmp slt i32 %t1030, 2
  %t1032 = zext i1 %t1031 to i32
  %t1033 = icmp ne i32 %t1032, 0
  br i1 %t1033, label %block_228, label %block_229
for.latch.228:
  %t1034 = load i32, ptr %lv.__i.32
  %t1035 = add i32 %t1034, 1
  store i32 %t1035, ptr %lv.__i.32
  br label %for.cond.228
block_226:
  br label %dowhile.cond.225
dowhile.cond.225:
  %t1036 = icmp ne i32 0, 0
  br i1 %t1036, label %block_225, label %block_227
block_227:
  %t1037 = load <2 x double>, ptr %lv.d0
  %t1038 = insertelement <2 x double> poison, double 0x4000000000000000, i64 0
  %t1039 = shufflevector <2 x double> %t1038, <2 x double> poison, <2 x i32> zeroinitializer
  %t1040 = fdiv <2 x double> %t1037, %t1039
  store <2 x double> %t1040, ptr %lv.d1
  store <2 x double> zeroinitializer, ptr %lv._clit_.15
  %t1041 = sext i32 0 to i64
  %t1042 = getelementptr double, ptr %lv._clit_.15, i64 %t1041
  store double 0x4000000000000000, ptr %t1042
  %t1043 = sext i32 1 to i64
  %t1044 = getelementptr double, ptr %lv._clit_.15, i64 %t1043
  store double 0x4000000000000000, ptr %t1044
  %t1045 = load <2 x double>, ptr %lv.d0
  %t1046 = load <2 x double>, ptr %lv._clit_.15
  %t1047 = fdiv <2 x double> %t1045, %t1046
  store <2 x double> %t1047, ptr %lv.d2
  br label %block_232
block_228:
  %t1048 = load i32, ptr %lv.__i.32
  %t1049 = sext i32 %t1048 to i64
  %t1050 = getelementptr double, ptr %lv.d1, i64 %t1049
  %t1051 = load double, ptr %t1050
  %t1052 = load i32, ptr %lv.__i.32
  %t1053 = sext i32 %t1052 to i64
  %t1054 = getelementptr double, ptr %lv.d2, i64 %t1053
  %t1055 = load double, ptr %t1054
  %t1056 = fcmp une double %t1051, %t1055
  %t1057 = zext i1 %t1056 to i32
  %t1058 = icmp ne i32 %t1057, 0
  br i1 %t1058, label %block_230, label %block_231
block_229:
  br label %block_226
block_230:
  %t1059 = call i32 @abort()
  br label %block_231
block_231:
  br label %for.latch.228
block_232:
  store i32 0, ptr %lv.__i.33
  br label %for.cond.235
for.cond.235:
  %t1060 = load i32, ptr %lv.__i.33
  %t1061 = icmp slt i32 %t1060, 2
  %t1062 = zext i1 %t1061 to i32
  %t1063 = icmp ne i32 %t1062, 0
  br i1 %t1063, label %block_235, label %block_236
for.latch.235:
  %t1064 = load i32, ptr %lv.__i.33
  %t1065 = add i32 %t1064, 1
  store i32 %t1065, ptr %lv.__i.33
  br label %for.cond.235
block_233:
  br label %dowhile.cond.232
dowhile.cond.232:
  %t1066 = icmp ne i32 0, 0
  br i1 %t1066, label %block_232, label %block_234
block_234:
  ret i32 0
block_235:
  %t1067 = load i32, ptr %lv.__i.33
  %t1068 = sext i32 %t1067 to i64
  %t1069 = getelementptr double, ptr %lv.d1, i64 %t1068
  %t1070 = load double, ptr %t1069
  %t1071 = load i32, ptr %lv.__i.33
  %t1072 = sext i32 %t1071 to i64
  %t1073 = getelementptr double, ptr %lv.d2, i64 %t1072
  %t1074 = load double, ptr %t1073
  %t1075 = fcmp une double %t1070, %t1074
  %t1076 = zext i1 %t1075 to i32
  %t1077 = icmp ne i32 %t1076, 0
  br i1 %t1077, label %block_237, label %block_238
block_236:
  br label %block_233
block_237:
  %t1078 = call i32 @abort()
  br label %block_238
block_238:
  br label %for.latch.235
}

