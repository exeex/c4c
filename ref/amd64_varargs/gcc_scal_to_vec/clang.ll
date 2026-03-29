; ModuleID = 'tests/c/external/gcc_torture/src/scal-to-vec1.c'
source_filename = "tests/c/external/gcc_torture/src/scal-to-vec1.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@one = dso_local global i32 1, align 4

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 noundef %0, ptr noundef %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca ptr, align 8
  %6 = alloca <8 x i16>, align 16
  %7 = alloca <8 x i16>, align 16
  %8 = alloca <4 x float>, align 16
  %9 = alloca <4 x float>, align 16
  %10 = alloca <4 x float>, align 16
  %11 = alloca <2 x double>, align 16
  %12 = alloca <2 x double>, align 16
  %13 = alloca <2 x double>, align 16
  %14 = alloca i32, align 4
  %15 = alloca i32, align 4
  %16 = alloca i32, align 4
  %17 = alloca i32, align 4
  %18 = alloca i32, align 4
  %19 = alloca i32, align 4
  %20 = alloca i32, align 4
  %21 = alloca i32, align 4
  %22 = alloca i32, align 4
  %23 = alloca i32, align 4
  %24 = alloca i32, align 4
  %25 = alloca i32, align 4
  %26 = alloca i32, align 4
  %27 = alloca i32, align 4
  %28 = alloca i32, align 4
  %29 = alloca i32, align 4
  %30 = alloca i32, align 4
  %31 = alloca i32, align 4
  %32 = alloca <4 x float>, align 16
  %33 = alloca i32, align 4
  %34 = alloca <4 x float>, align 16
  %35 = alloca i32, align 4
  %36 = alloca <4 x float>, align 16
  %37 = alloca i32, align 4
  %38 = alloca <4 x float>, align 16
  %39 = alloca i32, align 4
  %40 = alloca <4 x float>, align 16
  %41 = alloca i32, align 4
  %42 = alloca <4 x float>, align 16
  %43 = alloca i32, align 4
  %44 = alloca <4 x float>, align 16
  %45 = alloca i32, align 4
  %46 = alloca <4 x float>, align 16
  %47 = alloca i32, align 4
  %48 = alloca <2 x double>, align 16
  %49 = alloca i32, align 4
  %50 = alloca <2 x double>, align 16
  %51 = alloca i32, align 4
  %52 = alloca <2 x double>, align 16
  %53 = alloca i32, align 4
  %54 = alloca <2 x double>, align 16
  %55 = alloca i32, align 4
  %56 = alloca <2 x double>, align 16
  %57 = alloca i32, align 4
  %58 = alloca <2 x double>, align 16
  %59 = alloca i32, align 4
  %60 = alloca <2 x double>, align 16
  %61 = alloca i32, align 4
  %62 = alloca <2 x double>, align 16
  %63 = alloca i32, align 4
  store i32 0, ptr %3, align 4
  store i32 %0, ptr %4, align 4
  store ptr %1, ptr %5, align 8
  %64 = load volatile i32, ptr @one, align 4
  %65 = trunc i32 %64 to i16
  %66 = insertelement <8 x i16> poison, i16 %65, i32 0
  %67 = insertelement <8 x i16> %66, i16 1, i32 1
  %68 = insertelement <8 x i16> %67, i16 2, i32 2
  %69 = insertelement <8 x i16> %68, i16 3, i32 3
  %70 = insertelement <8 x i16> %69, i16 4, i32 4
  %71 = insertelement <8 x i16> %70, i16 5, i32 5
  %72 = insertelement <8 x i16> %71, i16 6, i32 6
  %73 = insertelement <8 x i16> %72, i16 7, i32 7
  store <8 x i16> %73, ptr %6, align 16
  store <4 x float> <float 1.000000e+00, float 2.000000e+00, float 3.000000e+00, float 4.000000e+00>, ptr %8, align 16
  store <2 x double> <double 1.000000e+00, double 2.000000e+00>, ptr %11, align 16
  %74 = load <8 x i16>, ptr %6, align 16
  %75 = add <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %74
  store <8 x i16> %75, ptr %7, align 16
  br label %76

76:                                               ; preds = %2
  store i32 0, ptr %14, align 4
  br label %77

77:                                               ; preds = %95, %76
  %78 = load i32, ptr %14, align 4
  %79 = icmp slt i32 %78, 8
  br i1 %79, label %80, label %98

80:                                               ; preds = %77
  %81 = load i32, ptr %14, align 4
  %82 = sext i32 %81 to i64
  %83 = getelementptr inbounds i16, ptr %7, i64 %82
  %84 = load i16, ptr %83, align 2
  %85 = sext i16 %84 to i32
  %86 = load i32, ptr %14, align 4
  %87 = sext i32 %86 to i64
  %88 = getelementptr inbounds i16, ptr %6, i64 %87
  %89 = load i16, ptr %88, align 2
  %90 = sext i16 %89 to i32
  %91 = add nsw i32 2, %90
  %92 = icmp ne i32 %85, %91
  br i1 %92, label %93, label %94

93:                                               ; preds = %80
  call void @abort() #2
  unreachable

94:                                               ; preds = %80
  br label %95

95:                                               ; preds = %94
  %96 = load i32, ptr %14, align 4
  %97 = add nsw i32 %96, 1
  store i32 %97, ptr %14, align 4
  br label %77

98:                                               ; preds = %77
  br label %99

99:                                               ; preds = %98
  %100 = load <8 x i16>, ptr %6, align 16
  %101 = sub <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %100
  store <8 x i16> %101, ptr %7, align 16
  br label %102

102:                                              ; preds = %99
  store i32 0, ptr %15, align 4
  br label %103

103:                                              ; preds = %121, %102
  %104 = load i32, ptr %15, align 4
  %105 = icmp slt i32 %104, 8
  br i1 %105, label %106, label %124

106:                                              ; preds = %103
  %107 = load i32, ptr %15, align 4
  %108 = sext i32 %107 to i64
  %109 = getelementptr inbounds i16, ptr %7, i64 %108
  %110 = load i16, ptr %109, align 2
  %111 = sext i16 %110 to i32
  %112 = load i32, ptr %15, align 4
  %113 = sext i32 %112 to i64
  %114 = getelementptr inbounds i16, ptr %6, i64 %113
  %115 = load i16, ptr %114, align 2
  %116 = sext i16 %115 to i32
  %117 = sub nsw i32 2, %116
  %118 = icmp ne i32 %111, %117
  br i1 %118, label %119, label %120

119:                                              ; preds = %106
  call void @abort() #2
  unreachable

120:                                              ; preds = %106
  br label %121

121:                                              ; preds = %120
  %122 = load i32, ptr %15, align 4
  %123 = add nsw i32 %122, 1
  store i32 %123, ptr %15, align 4
  br label %103

124:                                              ; preds = %103
  br label %125

125:                                              ; preds = %124
  %126 = load <8 x i16>, ptr %6, align 16
  %127 = mul <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %126
  store <8 x i16> %127, ptr %7, align 16
  br label %128

128:                                              ; preds = %125
  store i32 0, ptr %16, align 4
  br label %129

129:                                              ; preds = %147, %128
  %130 = load i32, ptr %16, align 4
  %131 = icmp slt i32 %130, 8
  br i1 %131, label %132, label %150

132:                                              ; preds = %129
  %133 = load i32, ptr %16, align 4
  %134 = sext i32 %133 to i64
  %135 = getelementptr inbounds i16, ptr %7, i64 %134
  %136 = load i16, ptr %135, align 2
  %137 = sext i16 %136 to i32
  %138 = load i32, ptr %16, align 4
  %139 = sext i32 %138 to i64
  %140 = getelementptr inbounds i16, ptr %6, i64 %139
  %141 = load i16, ptr %140, align 2
  %142 = sext i16 %141 to i32
  %143 = mul nsw i32 2, %142
  %144 = icmp ne i32 %137, %143
  br i1 %144, label %145, label %146

145:                                              ; preds = %132
  call void @abort() #2
  unreachable

146:                                              ; preds = %132
  br label %147

147:                                              ; preds = %146
  %148 = load i32, ptr %16, align 4
  %149 = add nsw i32 %148, 1
  store i32 %149, ptr %16, align 4
  br label %129

150:                                              ; preds = %129
  br label %151

151:                                              ; preds = %150
  %152 = load <8 x i16>, ptr %6, align 16
  %153 = sdiv <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %152
  store <8 x i16> %153, ptr %7, align 16
  br label %154

154:                                              ; preds = %151
  store i32 0, ptr %17, align 4
  br label %155

155:                                              ; preds = %173, %154
  %156 = load i32, ptr %17, align 4
  %157 = icmp slt i32 %156, 8
  br i1 %157, label %158, label %176

158:                                              ; preds = %155
  %159 = load i32, ptr %17, align 4
  %160 = sext i32 %159 to i64
  %161 = getelementptr inbounds i16, ptr %7, i64 %160
  %162 = load i16, ptr %161, align 2
  %163 = sext i16 %162 to i32
  %164 = load i32, ptr %17, align 4
  %165 = sext i32 %164 to i64
  %166 = getelementptr inbounds i16, ptr %6, i64 %165
  %167 = load i16, ptr %166, align 2
  %168 = sext i16 %167 to i32
  %169 = sdiv i32 2, %168
  %170 = icmp ne i32 %163, %169
  br i1 %170, label %171, label %172

171:                                              ; preds = %158
  call void @abort() #2
  unreachable

172:                                              ; preds = %158
  br label %173

173:                                              ; preds = %172
  %174 = load i32, ptr %17, align 4
  %175 = add nsw i32 %174, 1
  store i32 %175, ptr %17, align 4
  br label %155

176:                                              ; preds = %155
  br label %177

177:                                              ; preds = %176
  %178 = load <8 x i16>, ptr %6, align 16
  %179 = srem <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %178
  store <8 x i16> %179, ptr %7, align 16
  br label %180

180:                                              ; preds = %177
  store i32 0, ptr %18, align 4
  br label %181

181:                                              ; preds = %199, %180
  %182 = load i32, ptr %18, align 4
  %183 = icmp slt i32 %182, 8
  br i1 %183, label %184, label %202

184:                                              ; preds = %181
  %185 = load i32, ptr %18, align 4
  %186 = sext i32 %185 to i64
  %187 = getelementptr inbounds i16, ptr %7, i64 %186
  %188 = load i16, ptr %187, align 2
  %189 = sext i16 %188 to i32
  %190 = load i32, ptr %18, align 4
  %191 = sext i32 %190 to i64
  %192 = getelementptr inbounds i16, ptr %6, i64 %191
  %193 = load i16, ptr %192, align 2
  %194 = sext i16 %193 to i32
  %195 = srem i32 2, %194
  %196 = icmp ne i32 %189, %195
  br i1 %196, label %197, label %198

197:                                              ; preds = %184
  call void @abort() #2
  unreachable

198:                                              ; preds = %184
  br label %199

199:                                              ; preds = %198
  %200 = load i32, ptr %18, align 4
  %201 = add nsw i32 %200, 1
  store i32 %201, ptr %18, align 4
  br label %181

202:                                              ; preds = %181
  br label %203

203:                                              ; preds = %202
  %204 = load <8 x i16>, ptr %6, align 16
  %205 = xor <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %204
  store <8 x i16> %205, ptr %7, align 16
  br label %206

206:                                              ; preds = %203
  store i32 0, ptr %19, align 4
  br label %207

207:                                              ; preds = %225, %206
  %208 = load i32, ptr %19, align 4
  %209 = icmp slt i32 %208, 8
  br i1 %209, label %210, label %228

210:                                              ; preds = %207
  %211 = load i32, ptr %19, align 4
  %212 = sext i32 %211 to i64
  %213 = getelementptr inbounds i16, ptr %7, i64 %212
  %214 = load i16, ptr %213, align 2
  %215 = sext i16 %214 to i32
  %216 = load i32, ptr %19, align 4
  %217 = sext i32 %216 to i64
  %218 = getelementptr inbounds i16, ptr %6, i64 %217
  %219 = load i16, ptr %218, align 2
  %220 = sext i16 %219 to i32
  %221 = xor i32 2, %220
  %222 = icmp ne i32 %215, %221
  br i1 %222, label %223, label %224

223:                                              ; preds = %210
  call void @abort() #2
  unreachable

224:                                              ; preds = %210
  br label %225

225:                                              ; preds = %224
  %226 = load i32, ptr %19, align 4
  %227 = add nsw i32 %226, 1
  store i32 %227, ptr %19, align 4
  br label %207

228:                                              ; preds = %207
  br label %229

229:                                              ; preds = %228
  %230 = load <8 x i16>, ptr %6, align 16
  %231 = and <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %230
  store <8 x i16> %231, ptr %7, align 16
  br label %232

232:                                              ; preds = %229
  store i32 0, ptr %20, align 4
  br label %233

233:                                              ; preds = %251, %232
  %234 = load i32, ptr %20, align 4
  %235 = icmp slt i32 %234, 8
  br i1 %235, label %236, label %254

236:                                              ; preds = %233
  %237 = load i32, ptr %20, align 4
  %238 = sext i32 %237 to i64
  %239 = getelementptr inbounds i16, ptr %7, i64 %238
  %240 = load i16, ptr %239, align 2
  %241 = sext i16 %240 to i32
  %242 = load i32, ptr %20, align 4
  %243 = sext i32 %242 to i64
  %244 = getelementptr inbounds i16, ptr %6, i64 %243
  %245 = load i16, ptr %244, align 2
  %246 = sext i16 %245 to i32
  %247 = and i32 2, %246
  %248 = icmp ne i32 %241, %247
  br i1 %248, label %249, label %250

249:                                              ; preds = %236
  call void @abort() #2
  unreachable

250:                                              ; preds = %236
  br label %251

251:                                              ; preds = %250
  %252 = load i32, ptr %20, align 4
  %253 = add nsw i32 %252, 1
  store i32 %253, ptr %20, align 4
  br label %233

254:                                              ; preds = %233
  br label %255

255:                                              ; preds = %254
  %256 = load <8 x i16>, ptr %6, align 16
  %257 = or <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %256
  store <8 x i16> %257, ptr %7, align 16
  br label %258

258:                                              ; preds = %255
  store i32 0, ptr %21, align 4
  br label %259

259:                                              ; preds = %277, %258
  %260 = load i32, ptr %21, align 4
  %261 = icmp slt i32 %260, 8
  br i1 %261, label %262, label %280

262:                                              ; preds = %259
  %263 = load i32, ptr %21, align 4
  %264 = sext i32 %263 to i64
  %265 = getelementptr inbounds i16, ptr %7, i64 %264
  %266 = load i16, ptr %265, align 2
  %267 = sext i16 %266 to i32
  %268 = load i32, ptr %21, align 4
  %269 = sext i32 %268 to i64
  %270 = getelementptr inbounds i16, ptr %6, i64 %269
  %271 = load i16, ptr %270, align 2
  %272 = sext i16 %271 to i32
  %273 = or i32 2, %272
  %274 = icmp ne i32 %267, %273
  br i1 %274, label %275, label %276

275:                                              ; preds = %262
  call void @abort() #2
  unreachable

276:                                              ; preds = %262
  br label %277

277:                                              ; preds = %276
  %278 = load i32, ptr %21, align 4
  %279 = add nsw i32 %278, 1
  store i32 %279, ptr %21, align 4
  br label %259

280:                                              ; preds = %259
  br label %281

281:                                              ; preds = %280
  %282 = load <8 x i16>, ptr %6, align 16
  %283 = shl <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %282
  store <8 x i16> %283, ptr %7, align 16
  br label %284

284:                                              ; preds = %281
  store i32 0, ptr %22, align 4
  br label %285

285:                                              ; preds = %303, %284
  %286 = load i32, ptr %22, align 4
  %287 = icmp slt i32 %286, 8
  br i1 %287, label %288, label %306

288:                                              ; preds = %285
  %289 = load i32, ptr %22, align 4
  %290 = sext i32 %289 to i64
  %291 = getelementptr inbounds i16, ptr %7, i64 %290
  %292 = load i16, ptr %291, align 2
  %293 = sext i16 %292 to i32
  %294 = load i32, ptr %22, align 4
  %295 = sext i32 %294 to i64
  %296 = getelementptr inbounds i16, ptr %6, i64 %295
  %297 = load i16, ptr %296, align 2
  %298 = sext i16 %297 to i32
  %299 = shl i32 2, %298
  %300 = icmp ne i32 %293, %299
  br i1 %300, label %301, label %302

301:                                              ; preds = %288
  call void @abort() #2
  unreachable

302:                                              ; preds = %288
  br label %303

303:                                              ; preds = %302
  %304 = load i32, ptr %22, align 4
  %305 = add nsw i32 %304, 1
  store i32 %305, ptr %22, align 4
  br label %285

306:                                              ; preds = %285
  br label %307

307:                                              ; preds = %306
  %308 = load <8 x i16>, ptr %6, align 16
  %309 = ashr <8 x i16> <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>, %308
  store <8 x i16> %309, ptr %7, align 16
  br label %310

310:                                              ; preds = %307
  store i32 0, ptr %23, align 4
  br label %311

311:                                              ; preds = %329, %310
  %312 = load i32, ptr %23, align 4
  %313 = icmp slt i32 %312, 8
  br i1 %313, label %314, label %332

314:                                              ; preds = %311
  %315 = load i32, ptr %23, align 4
  %316 = sext i32 %315 to i64
  %317 = getelementptr inbounds i16, ptr %7, i64 %316
  %318 = load i16, ptr %317, align 2
  %319 = sext i16 %318 to i32
  %320 = load i32, ptr %23, align 4
  %321 = sext i32 %320 to i64
  %322 = getelementptr inbounds i16, ptr %6, i64 %321
  %323 = load i16, ptr %322, align 2
  %324 = sext i16 %323 to i32
  %325 = ashr i32 2, %324
  %326 = icmp ne i32 %319, %325
  br i1 %326, label %327, label %328

327:                                              ; preds = %314
  call void @abort() #2
  unreachable

328:                                              ; preds = %314
  br label %329

329:                                              ; preds = %328
  %330 = load i32, ptr %23, align 4
  %331 = add nsw i32 %330, 1
  store i32 %331, ptr %23, align 4
  br label %311

332:                                              ; preds = %311
  br label %333

333:                                              ; preds = %332
  %334 = load <8 x i16>, ptr %6, align 16
  %335 = add <8 x i16> %334, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %335, ptr %7, align 16
  br label %336

336:                                              ; preds = %333
  store i32 0, ptr %24, align 4
  br label %337

337:                                              ; preds = %355, %336
  %338 = load i32, ptr %24, align 4
  %339 = icmp slt i32 %338, 8
  br i1 %339, label %340, label %358

340:                                              ; preds = %337
  %341 = load i32, ptr %24, align 4
  %342 = sext i32 %341 to i64
  %343 = getelementptr inbounds i16, ptr %7, i64 %342
  %344 = load i16, ptr %343, align 2
  %345 = sext i16 %344 to i32
  %346 = load i32, ptr %24, align 4
  %347 = sext i32 %346 to i64
  %348 = getelementptr inbounds i16, ptr %6, i64 %347
  %349 = load i16, ptr %348, align 2
  %350 = sext i16 %349 to i32
  %351 = add nsw i32 %350, 2
  %352 = icmp ne i32 %345, %351
  br i1 %352, label %353, label %354

353:                                              ; preds = %340
  call void @abort() #2
  unreachable

354:                                              ; preds = %340
  br label %355

355:                                              ; preds = %354
  %356 = load i32, ptr %24, align 4
  %357 = add nsw i32 %356, 1
  store i32 %357, ptr %24, align 4
  br label %337

358:                                              ; preds = %337
  br label %359

359:                                              ; preds = %358
  %360 = load <8 x i16>, ptr %6, align 16
  %361 = sub <8 x i16> %360, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %361, ptr %7, align 16
  br label %362

362:                                              ; preds = %359
  store i32 0, ptr %25, align 4
  br label %363

363:                                              ; preds = %381, %362
  %364 = load i32, ptr %25, align 4
  %365 = icmp slt i32 %364, 8
  br i1 %365, label %366, label %384

366:                                              ; preds = %363
  %367 = load i32, ptr %25, align 4
  %368 = sext i32 %367 to i64
  %369 = getelementptr inbounds i16, ptr %7, i64 %368
  %370 = load i16, ptr %369, align 2
  %371 = sext i16 %370 to i32
  %372 = load i32, ptr %25, align 4
  %373 = sext i32 %372 to i64
  %374 = getelementptr inbounds i16, ptr %6, i64 %373
  %375 = load i16, ptr %374, align 2
  %376 = sext i16 %375 to i32
  %377 = sub nsw i32 %376, 2
  %378 = icmp ne i32 %371, %377
  br i1 %378, label %379, label %380

379:                                              ; preds = %366
  call void @abort() #2
  unreachable

380:                                              ; preds = %366
  br label %381

381:                                              ; preds = %380
  %382 = load i32, ptr %25, align 4
  %383 = add nsw i32 %382, 1
  store i32 %383, ptr %25, align 4
  br label %363

384:                                              ; preds = %363
  br label %385

385:                                              ; preds = %384
  %386 = load <8 x i16>, ptr %6, align 16
  %387 = mul <8 x i16> %386, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %387, ptr %7, align 16
  br label %388

388:                                              ; preds = %385
  store i32 0, ptr %26, align 4
  br label %389

389:                                              ; preds = %407, %388
  %390 = load i32, ptr %26, align 4
  %391 = icmp slt i32 %390, 8
  br i1 %391, label %392, label %410

392:                                              ; preds = %389
  %393 = load i32, ptr %26, align 4
  %394 = sext i32 %393 to i64
  %395 = getelementptr inbounds i16, ptr %7, i64 %394
  %396 = load i16, ptr %395, align 2
  %397 = sext i16 %396 to i32
  %398 = load i32, ptr %26, align 4
  %399 = sext i32 %398 to i64
  %400 = getelementptr inbounds i16, ptr %6, i64 %399
  %401 = load i16, ptr %400, align 2
  %402 = sext i16 %401 to i32
  %403 = mul nsw i32 %402, 2
  %404 = icmp ne i32 %397, %403
  br i1 %404, label %405, label %406

405:                                              ; preds = %392
  call void @abort() #2
  unreachable

406:                                              ; preds = %392
  br label %407

407:                                              ; preds = %406
  %408 = load i32, ptr %26, align 4
  %409 = add nsw i32 %408, 1
  store i32 %409, ptr %26, align 4
  br label %389

410:                                              ; preds = %389
  br label %411

411:                                              ; preds = %410
  %412 = load <8 x i16>, ptr %6, align 16
  %413 = sdiv <8 x i16> %412, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %413, ptr %7, align 16
  br label %414

414:                                              ; preds = %411
  store i32 0, ptr %27, align 4
  br label %415

415:                                              ; preds = %433, %414
  %416 = load i32, ptr %27, align 4
  %417 = icmp slt i32 %416, 8
  br i1 %417, label %418, label %436

418:                                              ; preds = %415
  %419 = load i32, ptr %27, align 4
  %420 = sext i32 %419 to i64
  %421 = getelementptr inbounds i16, ptr %7, i64 %420
  %422 = load i16, ptr %421, align 2
  %423 = sext i16 %422 to i32
  %424 = load i32, ptr %27, align 4
  %425 = sext i32 %424 to i64
  %426 = getelementptr inbounds i16, ptr %6, i64 %425
  %427 = load i16, ptr %426, align 2
  %428 = sext i16 %427 to i32
  %429 = sdiv i32 %428, 2
  %430 = icmp ne i32 %423, %429
  br i1 %430, label %431, label %432

431:                                              ; preds = %418
  call void @abort() #2
  unreachable

432:                                              ; preds = %418
  br label %433

433:                                              ; preds = %432
  %434 = load i32, ptr %27, align 4
  %435 = add nsw i32 %434, 1
  store i32 %435, ptr %27, align 4
  br label %415

436:                                              ; preds = %415
  br label %437

437:                                              ; preds = %436
  %438 = load <8 x i16>, ptr %6, align 16
  %439 = srem <8 x i16> %438, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %439, ptr %7, align 16
  br label %440

440:                                              ; preds = %437
  store i32 0, ptr %28, align 4
  br label %441

441:                                              ; preds = %459, %440
  %442 = load i32, ptr %28, align 4
  %443 = icmp slt i32 %442, 8
  br i1 %443, label %444, label %462

444:                                              ; preds = %441
  %445 = load i32, ptr %28, align 4
  %446 = sext i32 %445 to i64
  %447 = getelementptr inbounds i16, ptr %7, i64 %446
  %448 = load i16, ptr %447, align 2
  %449 = sext i16 %448 to i32
  %450 = load i32, ptr %28, align 4
  %451 = sext i32 %450 to i64
  %452 = getelementptr inbounds i16, ptr %6, i64 %451
  %453 = load i16, ptr %452, align 2
  %454 = sext i16 %453 to i32
  %455 = srem i32 %454, 2
  %456 = icmp ne i32 %449, %455
  br i1 %456, label %457, label %458

457:                                              ; preds = %444
  call void @abort() #2
  unreachable

458:                                              ; preds = %444
  br label %459

459:                                              ; preds = %458
  %460 = load i32, ptr %28, align 4
  %461 = add nsw i32 %460, 1
  store i32 %461, ptr %28, align 4
  br label %441

462:                                              ; preds = %441
  br label %463

463:                                              ; preds = %462
  %464 = load <8 x i16>, ptr %6, align 16
  %465 = xor <8 x i16> %464, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %465, ptr %7, align 16
  br label %466

466:                                              ; preds = %463
  store i32 0, ptr %29, align 4
  br label %467

467:                                              ; preds = %485, %466
  %468 = load i32, ptr %29, align 4
  %469 = icmp slt i32 %468, 8
  br i1 %469, label %470, label %488

470:                                              ; preds = %467
  %471 = load i32, ptr %29, align 4
  %472 = sext i32 %471 to i64
  %473 = getelementptr inbounds i16, ptr %7, i64 %472
  %474 = load i16, ptr %473, align 2
  %475 = sext i16 %474 to i32
  %476 = load i32, ptr %29, align 4
  %477 = sext i32 %476 to i64
  %478 = getelementptr inbounds i16, ptr %6, i64 %477
  %479 = load i16, ptr %478, align 2
  %480 = sext i16 %479 to i32
  %481 = xor i32 %480, 2
  %482 = icmp ne i32 %475, %481
  br i1 %482, label %483, label %484

483:                                              ; preds = %470
  call void @abort() #2
  unreachable

484:                                              ; preds = %470
  br label %485

485:                                              ; preds = %484
  %486 = load i32, ptr %29, align 4
  %487 = add nsw i32 %486, 1
  store i32 %487, ptr %29, align 4
  br label %467

488:                                              ; preds = %467
  br label %489

489:                                              ; preds = %488
  %490 = load <8 x i16>, ptr %6, align 16
  %491 = and <8 x i16> %490, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %491, ptr %7, align 16
  br label %492

492:                                              ; preds = %489
  store i32 0, ptr %30, align 4
  br label %493

493:                                              ; preds = %511, %492
  %494 = load i32, ptr %30, align 4
  %495 = icmp slt i32 %494, 8
  br i1 %495, label %496, label %514

496:                                              ; preds = %493
  %497 = load i32, ptr %30, align 4
  %498 = sext i32 %497 to i64
  %499 = getelementptr inbounds i16, ptr %7, i64 %498
  %500 = load i16, ptr %499, align 2
  %501 = sext i16 %500 to i32
  %502 = load i32, ptr %30, align 4
  %503 = sext i32 %502 to i64
  %504 = getelementptr inbounds i16, ptr %6, i64 %503
  %505 = load i16, ptr %504, align 2
  %506 = sext i16 %505 to i32
  %507 = and i32 %506, 2
  %508 = icmp ne i32 %501, %507
  br i1 %508, label %509, label %510

509:                                              ; preds = %496
  call void @abort() #2
  unreachable

510:                                              ; preds = %496
  br label %511

511:                                              ; preds = %510
  %512 = load i32, ptr %30, align 4
  %513 = add nsw i32 %512, 1
  store i32 %513, ptr %30, align 4
  br label %493

514:                                              ; preds = %493
  br label %515

515:                                              ; preds = %514
  %516 = load <8 x i16>, ptr %6, align 16
  %517 = or <8 x i16> %516, <i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2, i16 2>
  store <8 x i16> %517, ptr %7, align 16
  br label %518

518:                                              ; preds = %515
  store i32 0, ptr %31, align 4
  br label %519

519:                                              ; preds = %537, %518
  %520 = load i32, ptr %31, align 4
  %521 = icmp slt i32 %520, 8
  br i1 %521, label %522, label %540

522:                                              ; preds = %519
  %523 = load i32, ptr %31, align 4
  %524 = sext i32 %523 to i64
  %525 = getelementptr inbounds i16, ptr %7, i64 %524
  %526 = load i16, ptr %525, align 2
  %527 = sext i16 %526 to i32
  %528 = load i32, ptr %31, align 4
  %529 = sext i32 %528 to i64
  %530 = getelementptr inbounds i16, ptr %6, i64 %529
  %531 = load i16, ptr %530, align 2
  %532 = sext i16 %531 to i32
  %533 = or i32 %532, 2
  %534 = icmp ne i32 %527, %533
  br i1 %534, label %535, label %536

535:                                              ; preds = %522
  call void @abort() #2
  unreachable

536:                                              ; preds = %522
  br label %537

537:                                              ; preds = %536
  %538 = load i32, ptr %31, align 4
  %539 = add nsw i32 %538, 1
  store i32 %539, ptr %31, align 4
  br label %519

540:                                              ; preds = %519
  br label %541

541:                                              ; preds = %540
  %542 = load <4 x float>, ptr %8, align 16
  %543 = fadd <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, %542
  store <4 x float> %543, ptr %9, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %32, align 16
  %544 = load <4 x float>, ptr %32, align 16
  %545 = load <4 x float>, ptr %8, align 16
  %546 = fadd <4 x float> %544, %545
  store <4 x float> %546, ptr %10, align 16
  br label %547

547:                                              ; preds = %541
  store i32 0, ptr %33, align 4
  br label %548

548:                                              ; preds = %563, %547
  %549 = load i32, ptr %33, align 4
  %550 = icmp slt i32 %549, 4
  br i1 %550, label %551, label %566

551:                                              ; preds = %548
  %552 = load i32, ptr %33, align 4
  %553 = sext i32 %552 to i64
  %554 = getelementptr inbounds float, ptr %9, i64 %553
  %555 = load float, ptr %554, align 4
  %556 = load i32, ptr %33, align 4
  %557 = sext i32 %556 to i64
  %558 = getelementptr inbounds float, ptr %10, i64 %557
  %559 = load float, ptr %558, align 4
  %560 = fcmp une float %555, %559
  br i1 %560, label %561, label %562

561:                                              ; preds = %551
  call void @abort() #2
  unreachable

562:                                              ; preds = %551
  br label %563

563:                                              ; preds = %562
  %564 = load i32, ptr %33, align 4
  %565 = add nsw i32 %564, 1
  store i32 %565, ptr %33, align 4
  br label %548

566:                                              ; preds = %548
  br label %567

567:                                              ; preds = %566
  %568 = load <4 x float>, ptr %8, align 16
  %569 = fsub <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, %568
  store <4 x float> %569, ptr %9, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %34, align 16
  %570 = load <4 x float>, ptr %34, align 16
  %571 = load <4 x float>, ptr %8, align 16
  %572 = fsub <4 x float> %570, %571
  store <4 x float> %572, ptr %10, align 16
  br label %573

573:                                              ; preds = %567
  store i32 0, ptr %35, align 4
  br label %574

574:                                              ; preds = %589, %573
  %575 = load i32, ptr %35, align 4
  %576 = icmp slt i32 %575, 4
  br i1 %576, label %577, label %592

577:                                              ; preds = %574
  %578 = load i32, ptr %35, align 4
  %579 = sext i32 %578 to i64
  %580 = getelementptr inbounds float, ptr %9, i64 %579
  %581 = load float, ptr %580, align 4
  %582 = load i32, ptr %35, align 4
  %583 = sext i32 %582 to i64
  %584 = getelementptr inbounds float, ptr %10, i64 %583
  %585 = load float, ptr %584, align 4
  %586 = fcmp une float %581, %585
  br i1 %586, label %587, label %588

587:                                              ; preds = %577
  call void @abort() #2
  unreachable

588:                                              ; preds = %577
  br label %589

589:                                              ; preds = %588
  %590 = load i32, ptr %35, align 4
  %591 = add nsw i32 %590, 1
  store i32 %591, ptr %35, align 4
  br label %574

592:                                              ; preds = %574
  br label %593

593:                                              ; preds = %592
  %594 = load <4 x float>, ptr %8, align 16
  %595 = fmul <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, %594
  store <4 x float> %595, ptr %9, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %36, align 16
  %596 = load <4 x float>, ptr %36, align 16
  %597 = load <4 x float>, ptr %8, align 16
  %598 = fmul <4 x float> %596, %597
  store <4 x float> %598, ptr %10, align 16
  br label %599

599:                                              ; preds = %593
  store i32 0, ptr %37, align 4
  br label %600

600:                                              ; preds = %615, %599
  %601 = load i32, ptr %37, align 4
  %602 = icmp slt i32 %601, 4
  br i1 %602, label %603, label %618

603:                                              ; preds = %600
  %604 = load i32, ptr %37, align 4
  %605 = sext i32 %604 to i64
  %606 = getelementptr inbounds float, ptr %9, i64 %605
  %607 = load float, ptr %606, align 4
  %608 = load i32, ptr %37, align 4
  %609 = sext i32 %608 to i64
  %610 = getelementptr inbounds float, ptr %10, i64 %609
  %611 = load float, ptr %610, align 4
  %612 = fcmp une float %607, %611
  br i1 %612, label %613, label %614

613:                                              ; preds = %603
  call void @abort() #2
  unreachable

614:                                              ; preds = %603
  br label %615

615:                                              ; preds = %614
  %616 = load i32, ptr %37, align 4
  %617 = add nsw i32 %616, 1
  store i32 %617, ptr %37, align 4
  br label %600

618:                                              ; preds = %600
  br label %619

619:                                              ; preds = %618
  %620 = load <4 x float>, ptr %8, align 16
  %621 = fdiv <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, %620
  store <4 x float> %621, ptr %9, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %38, align 16
  %622 = load <4 x float>, ptr %38, align 16
  %623 = load <4 x float>, ptr %8, align 16
  %624 = fdiv <4 x float> %622, %623
  store <4 x float> %624, ptr %10, align 16
  br label %625

625:                                              ; preds = %619
  store i32 0, ptr %39, align 4
  br label %626

626:                                              ; preds = %641, %625
  %627 = load i32, ptr %39, align 4
  %628 = icmp slt i32 %627, 4
  br i1 %628, label %629, label %644

629:                                              ; preds = %626
  %630 = load i32, ptr %39, align 4
  %631 = sext i32 %630 to i64
  %632 = getelementptr inbounds float, ptr %9, i64 %631
  %633 = load float, ptr %632, align 4
  %634 = load i32, ptr %39, align 4
  %635 = sext i32 %634 to i64
  %636 = getelementptr inbounds float, ptr %10, i64 %635
  %637 = load float, ptr %636, align 4
  %638 = fcmp une float %633, %637
  br i1 %638, label %639, label %640

639:                                              ; preds = %629
  call void @abort() #2
  unreachable

640:                                              ; preds = %629
  br label %641

641:                                              ; preds = %640
  %642 = load i32, ptr %39, align 4
  %643 = add nsw i32 %642, 1
  store i32 %643, ptr %39, align 4
  br label %626

644:                                              ; preds = %626
  br label %645

645:                                              ; preds = %644
  %646 = load <4 x float>, ptr %8, align 16
  %647 = fadd <4 x float> %646, <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
  store <4 x float> %647, ptr %9, align 16
  %648 = load <4 x float>, ptr %8, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %40, align 16
  %649 = load <4 x float>, ptr %40, align 16
  %650 = fadd <4 x float> %648, %649
  store <4 x float> %650, ptr %10, align 16
  br label %651

651:                                              ; preds = %645
  store i32 0, ptr %41, align 4
  br label %652

652:                                              ; preds = %667, %651
  %653 = load i32, ptr %41, align 4
  %654 = icmp slt i32 %653, 4
  br i1 %654, label %655, label %670

655:                                              ; preds = %652
  %656 = load i32, ptr %41, align 4
  %657 = sext i32 %656 to i64
  %658 = getelementptr inbounds float, ptr %9, i64 %657
  %659 = load float, ptr %658, align 4
  %660 = load i32, ptr %41, align 4
  %661 = sext i32 %660 to i64
  %662 = getelementptr inbounds float, ptr %10, i64 %661
  %663 = load float, ptr %662, align 4
  %664 = fcmp une float %659, %663
  br i1 %664, label %665, label %666

665:                                              ; preds = %655
  call void @abort() #2
  unreachable

666:                                              ; preds = %655
  br label %667

667:                                              ; preds = %666
  %668 = load i32, ptr %41, align 4
  %669 = add nsw i32 %668, 1
  store i32 %669, ptr %41, align 4
  br label %652

670:                                              ; preds = %652
  br label %671

671:                                              ; preds = %670
  %672 = load <4 x float>, ptr %8, align 16
  %673 = fsub <4 x float> %672, <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
  store <4 x float> %673, ptr %9, align 16
  %674 = load <4 x float>, ptr %8, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %42, align 16
  %675 = load <4 x float>, ptr %42, align 16
  %676 = fsub <4 x float> %674, %675
  store <4 x float> %676, ptr %10, align 16
  br label %677

677:                                              ; preds = %671
  store i32 0, ptr %43, align 4
  br label %678

678:                                              ; preds = %693, %677
  %679 = load i32, ptr %43, align 4
  %680 = icmp slt i32 %679, 4
  br i1 %680, label %681, label %696

681:                                              ; preds = %678
  %682 = load i32, ptr %43, align 4
  %683 = sext i32 %682 to i64
  %684 = getelementptr inbounds float, ptr %9, i64 %683
  %685 = load float, ptr %684, align 4
  %686 = load i32, ptr %43, align 4
  %687 = sext i32 %686 to i64
  %688 = getelementptr inbounds float, ptr %10, i64 %687
  %689 = load float, ptr %688, align 4
  %690 = fcmp une float %685, %689
  br i1 %690, label %691, label %692

691:                                              ; preds = %681
  call void @abort() #2
  unreachable

692:                                              ; preds = %681
  br label %693

693:                                              ; preds = %692
  %694 = load i32, ptr %43, align 4
  %695 = add nsw i32 %694, 1
  store i32 %695, ptr %43, align 4
  br label %678

696:                                              ; preds = %678
  br label %697

697:                                              ; preds = %696
  %698 = load <4 x float>, ptr %8, align 16
  %699 = fmul <4 x float> %698, <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
  store <4 x float> %699, ptr %9, align 16
  %700 = load <4 x float>, ptr %8, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %44, align 16
  %701 = load <4 x float>, ptr %44, align 16
  %702 = fmul <4 x float> %700, %701
  store <4 x float> %702, ptr %10, align 16
  br label %703

703:                                              ; preds = %697
  store i32 0, ptr %45, align 4
  br label %704

704:                                              ; preds = %719, %703
  %705 = load i32, ptr %45, align 4
  %706 = icmp slt i32 %705, 4
  br i1 %706, label %707, label %722

707:                                              ; preds = %704
  %708 = load i32, ptr %45, align 4
  %709 = sext i32 %708 to i64
  %710 = getelementptr inbounds float, ptr %9, i64 %709
  %711 = load float, ptr %710, align 4
  %712 = load i32, ptr %45, align 4
  %713 = sext i32 %712 to i64
  %714 = getelementptr inbounds float, ptr %10, i64 %713
  %715 = load float, ptr %714, align 4
  %716 = fcmp une float %711, %715
  br i1 %716, label %717, label %718

717:                                              ; preds = %707
  call void @abort() #2
  unreachable

718:                                              ; preds = %707
  br label %719

719:                                              ; preds = %718
  %720 = load i32, ptr %45, align 4
  %721 = add nsw i32 %720, 1
  store i32 %721, ptr %45, align 4
  br label %704

722:                                              ; preds = %704
  br label %723

723:                                              ; preds = %722
  %724 = load <4 x float>, ptr %8, align 16
  %725 = fdiv <4 x float> %724, <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>
  store <4 x float> %725, ptr %9, align 16
  %726 = load <4 x float>, ptr %8, align 16
  store <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>, ptr %46, align 16
  %727 = load <4 x float>, ptr %46, align 16
  %728 = fdiv <4 x float> %726, %727
  store <4 x float> %728, ptr %10, align 16
  br label %729

729:                                              ; preds = %723
  store i32 0, ptr %47, align 4
  br label %730

730:                                              ; preds = %745, %729
  %731 = load i32, ptr %47, align 4
  %732 = icmp slt i32 %731, 4
  br i1 %732, label %733, label %748

733:                                              ; preds = %730
  %734 = load i32, ptr %47, align 4
  %735 = sext i32 %734 to i64
  %736 = getelementptr inbounds float, ptr %9, i64 %735
  %737 = load float, ptr %736, align 4
  %738 = load i32, ptr %47, align 4
  %739 = sext i32 %738 to i64
  %740 = getelementptr inbounds float, ptr %10, i64 %739
  %741 = load float, ptr %740, align 4
  %742 = fcmp une float %737, %741
  br i1 %742, label %743, label %744

743:                                              ; preds = %733
  call void @abort() #2
  unreachable

744:                                              ; preds = %733
  br label %745

745:                                              ; preds = %744
  %746 = load i32, ptr %47, align 4
  %747 = add nsw i32 %746, 1
  store i32 %747, ptr %47, align 4
  br label %730

748:                                              ; preds = %730
  br label %749

749:                                              ; preds = %748
  %750 = load <2 x double>, ptr %11, align 16
  %751 = fadd <2 x double> <double 2.000000e+00, double 2.000000e+00>, %750
  store <2 x double> %751, ptr %12, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %48, align 16
  %752 = load <2 x double>, ptr %48, align 16
  %753 = load <2 x double>, ptr %11, align 16
  %754 = fadd <2 x double> %752, %753
  store <2 x double> %754, ptr %13, align 16
  br label %755

755:                                              ; preds = %749
  store i32 0, ptr %49, align 4
  br label %756

756:                                              ; preds = %771, %755
  %757 = load i32, ptr %49, align 4
  %758 = icmp slt i32 %757, 2
  br i1 %758, label %759, label %774

759:                                              ; preds = %756
  %760 = load i32, ptr %49, align 4
  %761 = sext i32 %760 to i64
  %762 = getelementptr inbounds double, ptr %12, i64 %761
  %763 = load double, ptr %762, align 8
  %764 = load i32, ptr %49, align 4
  %765 = sext i32 %764 to i64
  %766 = getelementptr inbounds double, ptr %13, i64 %765
  %767 = load double, ptr %766, align 8
  %768 = fcmp une double %763, %767
  br i1 %768, label %769, label %770

769:                                              ; preds = %759
  call void @abort() #2
  unreachable

770:                                              ; preds = %759
  br label %771

771:                                              ; preds = %770
  %772 = load i32, ptr %49, align 4
  %773 = add nsw i32 %772, 1
  store i32 %773, ptr %49, align 4
  br label %756

774:                                              ; preds = %756
  br label %775

775:                                              ; preds = %774
  %776 = load <2 x double>, ptr %11, align 16
  %777 = fsub <2 x double> <double 2.000000e+00, double 2.000000e+00>, %776
  store <2 x double> %777, ptr %12, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %50, align 16
  %778 = load <2 x double>, ptr %50, align 16
  %779 = load <2 x double>, ptr %11, align 16
  %780 = fsub <2 x double> %778, %779
  store <2 x double> %780, ptr %13, align 16
  br label %781

781:                                              ; preds = %775
  store i32 0, ptr %51, align 4
  br label %782

782:                                              ; preds = %797, %781
  %783 = load i32, ptr %51, align 4
  %784 = icmp slt i32 %783, 2
  br i1 %784, label %785, label %800

785:                                              ; preds = %782
  %786 = load i32, ptr %51, align 4
  %787 = sext i32 %786 to i64
  %788 = getelementptr inbounds double, ptr %12, i64 %787
  %789 = load double, ptr %788, align 8
  %790 = load i32, ptr %51, align 4
  %791 = sext i32 %790 to i64
  %792 = getelementptr inbounds double, ptr %13, i64 %791
  %793 = load double, ptr %792, align 8
  %794 = fcmp une double %789, %793
  br i1 %794, label %795, label %796

795:                                              ; preds = %785
  call void @abort() #2
  unreachable

796:                                              ; preds = %785
  br label %797

797:                                              ; preds = %796
  %798 = load i32, ptr %51, align 4
  %799 = add nsw i32 %798, 1
  store i32 %799, ptr %51, align 4
  br label %782

800:                                              ; preds = %782
  br label %801

801:                                              ; preds = %800
  %802 = load <2 x double>, ptr %11, align 16
  %803 = fmul <2 x double> <double 2.000000e+00, double 2.000000e+00>, %802
  store <2 x double> %803, ptr %12, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %52, align 16
  %804 = load <2 x double>, ptr %52, align 16
  %805 = load <2 x double>, ptr %11, align 16
  %806 = fmul <2 x double> %804, %805
  store <2 x double> %806, ptr %13, align 16
  br label %807

807:                                              ; preds = %801
  store i32 0, ptr %53, align 4
  br label %808

808:                                              ; preds = %823, %807
  %809 = load i32, ptr %53, align 4
  %810 = icmp slt i32 %809, 2
  br i1 %810, label %811, label %826

811:                                              ; preds = %808
  %812 = load i32, ptr %53, align 4
  %813 = sext i32 %812 to i64
  %814 = getelementptr inbounds double, ptr %12, i64 %813
  %815 = load double, ptr %814, align 8
  %816 = load i32, ptr %53, align 4
  %817 = sext i32 %816 to i64
  %818 = getelementptr inbounds double, ptr %13, i64 %817
  %819 = load double, ptr %818, align 8
  %820 = fcmp une double %815, %819
  br i1 %820, label %821, label %822

821:                                              ; preds = %811
  call void @abort() #2
  unreachable

822:                                              ; preds = %811
  br label %823

823:                                              ; preds = %822
  %824 = load i32, ptr %53, align 4
  %825 = add nsw i32 %824, 1
  store i32 %825, ptr %53, align 4
  br label %808

826:                                              ; preds = %808
  br label %827

827:                                              ; preds = %826
  %828 = load <2 x double>, ptr %11, align 16
  %829 = fdiv <2 x double> <double 2.000000e+00, double 2.000000e+00>, %828
  store <2 x double> %829, ptr %12, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %54, align 16
  %830 = load <2 x double>, ptr %54, align 16
  %831 = load <2 x double>, ptr %11, align 16
  %832 = fdiv <2 x double> %830, %831
  store <2 x double> %832, ptr %13, align 16
  br label %833

833:                                              ; preds = %827
  store i32 0, ptr %55, align 4
  br label %834

834:                                              ; preds = %849, %833
  %835 = load i32, ptr %55, align 4
  %836 = icmp slt i32 %835, 2
  br i1 %836, label %837, label %852

837:                                              ; preds = %834
  %838 = load i32, ptr %55, align 4
  %839 = sext i32 %838 to i64
  %840 = getelementptr inbounds double, ptr %12, i64 %839
  %841 = load double, ptr %840, align 8
  %842 = load i32, ptr %55, align 4
  %843 = sext i32 %842 to i64
  %844 = getelementptr inbounds double, ptr %13, i64 %843
  %845 = load double, ptr %844, align 8
  %846 = fcmp une double %841, %845
  br i1 %846, label %847, label %848

847:                                              ; preds = %837
  call void @abort() #2
  unreachable

848:                                              ; preds = %837
  br label %849

849:                                              ; preds = %848
  %850 = load i32, ptr %55, align 4
  %851 = add nsw i32 %850, 1
  store i32 %851, ptr %55, align 4
  br label %834

852:                                              ; preds = %834
  br label %853

853:                                              ; preds = %852
  %854 = load <2 x double>, ptr %11, align 16
  %855 = fadd <2 x double> %854, <double 2.000000e+00, double 2.000000e+00>
  store <2 x double> %855, ptr %12, align 16
  %856 = load <2 x double>, ptr %11, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %56, align 16
  %857 = load <2 x double>, ptr %56, align 16
  %858 = fadd <2 x double> %856, %857
  store <2 x double> %858, ptr %13, align 16
  br label %859

859:                                              ; preds = %853
  store i32 0, ptr %57, align 4
  br label %860

860:                                              ; preds = %875, %859
  %861 = load i32, ptr %57, align 4
  %862 = icmp slt i32 %861, 2
  br i1 %862, label %863, label %878

863:                                              ; preds = %860
  %864 = load i32, ptr %57, align 4
  %865 = sext i32 %864 to i64
  %866 = getelementptr inbounds double, ptr %12, i64 %865
  %867 = load double, ptr %866, align 8
  %868 = load i32, ptr %57, align 4
  %869 = sext i32 %868 to i64
  %870 = getelementptr inbounds double, ptr %13, i64 %869
  %871 = load double, ptr %870, align 8
  %872 = fcmp une double %867, %871
  br i1 %872, label %873, label %874

873:                                              ; preds = %863
  call void @abort() #2
  unreachable

874:                                              ; preds = %863
  br label %875

875:                                              ; preds = %874
  %876 = load i32, ptr %57, align 4
  %877 = add nsw i32 %876, 1
  store i32 %877, ptr %57, align 4
  br label %860

878:                                              ; preds = %860
  br label %879

879:                                              ; preds = %878
  %880 = load <2 x double>, ptr %11, align 16
  %881 = fsub <2 x double> %880, <double 2.000000e+00, double 2.000000e+00>
  store <2 x double> %881, ptr %12, align 16
  %882 = load <2 x double>, ptr %11, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %58, align 16
  %883 = load <2 x double>, ptr %58, align 16
  %884 = fsub <2 x double> %882, %883
  store <2 x double> %884, ptr %13, align 16
  br label %885

885:                                              ; preds = %879
  store i32 0, ptr %59, align 4
  br label %886

886:                                              ; preds = %901, %885
  %887 = load i32, ptr %59, align 4
  %888 = icmp slt i32 %887, 2
  br i1 %888, label %889, label %904

889:                                              ; preds = %886
  %890 = load i32, ptr %59, align 4
  %891 = sext i32 %890 to i64
  %892 = getelementptr inbounds double, ptr %12, i64 %891
  %893 = load double, ptr %892, align 8
  %894 = load i32, ptr %59, align 4
  %895 = sext i32 %894 to i64
  %896 = getelementptr inbounds double, ptr %13, i64 %895
  %897 = load double, ptr %896, align 8
  %898 = fcmp une double %893, %897
  br i1 %898, label %899, label %900

899:                                              ; preds = %889
  call void @abort() #2
  unreachable

900:                                              ; preds = %889
  br label %901

901:                                              ; preds = %900
  %902 = load i32, ptr %59, align 4
  %903 = add nsw i32 %902, 1
  store i32 %903, ptr %59, align 4
  br label %886

904:                                              ; preds = %886
  br label %905

905:                                              ; preds = %904
  %906 = load <2 x double>, ptr %11, align 16
  %907 = fmul <2 x double> %906, <double 2.000000e+00, double 2.000000e+00>
  store <2 x double> %907, ptr %12, align 16
  %908 = load <2 x double>, ptr %11, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %60, align 16
  %909 = load <2 x double>, ptr %60, align 16
  %910 = fmul <2 x double> %908, %909
  store <2 x double> %910, ptr %13, align 16
  br label %911

911:                                              ; preds = %905
  store i32 0, ptr %61, align 4
  br label %912

912:                                              ; preds = %927, %911
  %913 = load i32, ptr %61, align 4
  %914 = icmp slt i32 %913, 2
  br i1 %914, label %915, label %930

915:                                              ; preds = %912
  %916 = load i32, ptr %61, align 4
  %917 = sext i32 %916 to i64
  %918 = getelementptr inbounds double, ptr %12, i64 %917
  %919 = load double, ptr %918, align 8
  %920 = load i32, ptr %61, align 4
  %921 = sext i32 %920 to i64
  %922 = getelementptr inbounds double, ptr %13, i64 %921
  %923 = load double, ptr %922, align 8
  %924 = fcmp une double %919, %923
  br i1 %924, label %925, label %926

925:                                              ; preds = %915
  call void @abort() #2
  unreachable

926:                                              ; preds = %915
  br label %927

927:                                              ; preds = %926
  %928 = load i32, ptr %61, align 4
  %929 = add nsw i32 %928, 1
  store i32 %929, ptr %61, align 4
  br label %912

930:                                              ; preds = %912
  br label %931

931:                                              ; preds = %930
  %932 = load <2 x double>, ptr %11, align 16
  %933 = fdiv <2 x double> %932, <double 2.000000e+00, double 2.000000e+00>
  store <2 x double> %933, ptr %12, align 16
  %934 = load <2 x double>, ptr %11, align 16
  store <2 x double> <double 2.000000e+00, double 2.000000e+00>, ptr %62, align 16
  %935 = load <2 x double>, ptr %62, align 16
  %936 = fdiv <2 x double> %934, %935
  store <2 x double> %936, ptr %13, align 16
  br label %937

937:                                              ; preds = %931
  store i32 0, ptr %63, align 4
  br label %938

938:                                              ; preds = %953, %937
  %939 = load i32, ptr %63, align 4
  %940 = icmp slt i32 %939, 2
  br i1 %940, label %941, label %956

941:                                              ; preds = %938
  %942 = load i32, ptr %63, align 4
  %943 = sext i32 %942 to i64
  %944 = getelementptr inbounds double, ptr %12, i64 %943
  %945 = load double, ptr %944, align 8
  %946 = load i32, ptr %63, align 4
  %947 = sext i32 %946 to i64
  %948 = getelementptr inbounds double, ptr %13, i64 %947
  %949 = load double, ptr %948, align 8
  %950 = fcmp une double %945, %949
  br i1 %950, label %951, label %952

951:                                              ; preds = %941
  call void @abort() #2
  unreachable

952:                                              ; preds = %941
  br label %953

953:                                              ; preds = %952
  %954 = load i32, ptr %63, align 4
  %955 = add nsw i32 %954, 1
  store i32 %955, ptr %63, align 4
  br label %938

956:                                              ; preds = %938
  br label %957

957:                                              ; preds = %956
  ret i32 0
}

; Function Attrs: noreturn nounwind
declare void @abort() #1

attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { noreturn nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { noreturn nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Debian clang version 19.1.7 (3+b1)"}
