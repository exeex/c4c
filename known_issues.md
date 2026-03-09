# Known Issues

## 2026-03-09

### 1) `pr80421.c.c2ll.bin` 背景掛住 + CPU 100%（已修復）
- Case: `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/pr80421.c`
- 產物: `build/llvm_gcc_c_torture/pr80421.c.c2ll.bin`
- 原始現象:
  - `timeout 2s .../pr80421.c.c2ll.bin` -> exit `124`（超時），無任何輸出。
  - 同一份 source 用 `clang -O0` 直接編譯執行可正常結束（exit `0`）。
- 根因:
  - 在 `build/llvm_gcc_c_torture/pr80421.c.ll`，`f = c + 390` 被產成：
    - `%t1682 = getelementptr ptr, ptr %t1680, i64 %t1681`
  - 這裡 element type 應該是 `i8`，但實際用 `ptr`，造成指標位移步長錯誤。
  - 後續 `switch (f[g])` 會落到 default path（`block_16`）且不會遞減 `i/j`，`while (i > 0)` 無法收斂，形成 busy loop。
- 修正:
  - pointer arithmetic 的 GEP element type 推導改為正確 array decay（此 case 變為 `getelementptr i8`）。
  - `ctest -R llvm_gcc_c_torture_pr80421_c` 已通過。

### 2) `c_testsuite_tests_single_exec_00205_c`（ID 299）（已修復）
- Case: `tests/c-testsuite/tests/single-exec/00205.c`
- 原始失敗型態: `RUNTIME_MISMATCH`
- 根因:
  - 全域 array-of-structs 初始化缺少 brace elision 支援（`emit_const_array` / `emit_const_struct`）。
  - `PT cases[] = { v1, v2, ..., v63 }` 每個 scalar 被當成一個 array element，而非 7 個一組填入 struct。
- 修正:
  - 新增 `emit_const_from_flat()` cursor-based 遞迴消耗，處理 struct/array 的 brace elision。
  - `emit_const_struct` 加入 `use_cursor` 偵測（items > fields 或 scalar → aggregate field）。
  - `resolve_array_ts` 修正 unsized array 推導（全 scalar 時除以 `flat_scalar_count`）。
  - 另外修正 compound literal 在 init list 中的處理（`lower_init_list` 辨識 `NK_COMPOUND_LIT`→InitList）。
  - 額外修正 5 個 llvm_gcc_c_torture tests: 20000801_3, 20090113_2, 20090113_3, pr86714, struct_ini_1。

### 3) `c_testsuite_tests_single_exec_00216_c`（ID 310）
- Case: `tests/c-testsuite/tests/single-exec/00216.c`
- 失敗型態: `RUNTIME_NONZERO`（segfault）
- 進度:
  - 先前 BACKEND_FAIL（`load %struct.contains_empty` unsized）已修正：空 struct 現在會 emit `type {}`。
  - 全域 struct init brace elision（`@gu`, `@gv`, `@gv2` 等）已修正。
  - compound literal init（`@gs = ((struct S){1,2,3,4})`）已修正（`lower_global_init` 辨識 `NK_COMPOUND_LIT`）。
  - 目前 segfault，尚需調查：
    - union init（`@guv`、`@guv2`、`@guv3`）全為 zeroinitializer。
    - `@phdr` 巢狀 anonymous union/struct init 全為零。
    - `@gw` flex array member init 不完整。
    - `sizeof` 可能對部分 struct 回傳錯誤值（所有 print_ call 都用 i64 4）。

### 4) `c_testsuite_tests_single_exec_00219_c`（ID 313）（已修復）
- Case: `tests/c-testsuite/tests/single-exec/00219.c`
- 原始失敗型態: `BACKEND_FAIL`
- 原始報錯:
  - `.../00219.c.ll:224:13: error: integer constant must have integer type`
  - `call void 0()`
- 根因與修正:
  - `_Generic` controlling expression 型別推導不完整，且 type compatibility 規則過嚴/不一致，導致分支選擇錯誤。
  - 補上 `_Generic` 型別推導與 compatibility 修正後，`ctest -R c_testsuite_tests_single_exec_00219_c` 已通過。

### 5) `c_testsuite_tests_single_exec_00204_c`（ID 315）
- Case: `tests/c-testsuite/tests/single-exec/00204.c`
- 失敗型態: `BACKEND_FAIL`（clang codegen crash）
- 現象:
  - AArch64 codegen 階段崩潰：`Running pass 'RegBankSelect' on function '@myprintf'`
  - `clang: error: unable to execute command: Segmentation fault`
- 推測:
  - 這支測試包含大量 AArch64 ABI / `va_arg` 結構傳參壓力，當前生成的 IR 觸發 clang backend crash（可能為前端輸出 IR 的型別/調用約定不一致）。
