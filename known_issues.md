# Known Issues

## 2026-03-09

### 1) `pr80421.c.c2ll.bin` 背景掛住 + CPU 100%
- Case: `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/pr80421.c`
- 產物: `build/llvm_gcc_c_torture/pr80421.c.c2ll.bin`
- 現象:
  - `timeout 2s .../pr80421.c.c2ll.bin` -> exit `124`（超時），無任何輸出。
  - 同一份 source 用 `clang -O0` 直接編譯執行可正常結束（exit `0`）。
- 初步分析:
  - 在 `build/llvm_gcc_c_torture/pr80421.c.ll`，`f = c + 390` 被產成：
    - `%t1682 = getelementptr ptr, ptr %t1680, i64 %t1681`
  - 這裡 element type 應該是 `i8`，但實際用 `ptr`，造成指標位移步長錯誤。
  - 後續 `switch (f[g])` 會落到 default path（`block_16`）且不會遞減 `i/j`，`while (i > 0)` 無法收斂，形成 busy loop。

### 2) `c_testsuite_tests_single_exec_00205_c`（ID 299）
- Case: `tests/c-testsuite/tests/single-exec/00205.c`
- 失敗型態: `RUNTIME_MISMATCH`
- 現象:
  - 預期是多層 aggregate 初始化結果。
  - 實際輸出顯示大量欄位被清成 `0`，且資料看起來被錯誤平鋪/位移（例如不同 `cases[n].c[0]` 連續吃到原本同一筆資料的前幾個 initializer）。
- 推測:
  - aggregate initializer（含大量常量表達式）flatten / brace 對齊規則有回歸，導致欄位填值次序錯誤。

### 3) `c_testsuite_tests_single_exec_00216_c`（ID 310）
- Case: `tests/c-testsuite/tests/single-exec/00216.c`
- 失敗型態: `BACKEND_FAIL`
- clang 報錯:
  - `.../00216.c.ll:379:19: error: integer constant must have integer type`
  - `store %struct.S 5, ptr %t18`
- 推測:
  - 巢狀 struct 初始化 lowering 產生了型別不合法的 `store`（把整個 `%struct.S` 當成整數常量 `5` 存入）。

### 4) `c_testsuite_tests_single_exec_00219_c`（ID 313）
- Case: `tests/c-testsuite/tests/single-exec/00219.c`
- 失敗型態: `BACKEND_FAIL`
- clang 報錯:
  - `.../00219.c.ll:224:13: error: integer constant must have integer type`
  - `call void 0()`
- 推測:
  - `_Generic` 選擇結果在 function-designator / callable expression lowering 時被錯誤降成整數常量 `0`，最後生成非法 call IR。

### 5) `c_testsuite_tests_single_exec_00204_c`（ID 315）
- Case: `tests/c-testsuite/tests/single-exec/00204.c`
- 失敗型態: `BACKEND_FAIL`（clang codegen crash）
- 現象:
  - AArch64 codegen 階段崩潰：`Running pass 'RegBankSelect' on function '@myprintf'`
  - `clang: error: unable to execute command: Segmentation fault`
- 推測:
  - 這支測試包含大量 AArch64 ABI / `va_arg` 結構傳參壓力，當前生成的 IR 觸發 clang backend crash（可能為前端輸出 IR 的型別/調用約定不一致）。
