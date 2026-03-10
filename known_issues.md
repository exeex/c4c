# Known Issues

## 2026-03-09

### 1) `c_testsuite_tests_single_exec_00216_c`（ID 310）
- Case: `tests/c-testsuite/tests/single-exec/00216.c`
- 目前失敗型態: `RUNTIME_MISMATCH`
- 進度:
  - 已修正原本 `RUNTIME_NONZERO` segfault（`test_compound_with_relocs`）。
  - 已修正 `sizeof` 一律落成 4 的主要問題。
  - 已補 union global init 的 byte-level emission（`@guv/@guv2` 已正確）。
  - 尚未解:
    - `guv3`（anonymous member designator）仍初始化錯誤。
    - `phdr` 巢狀 anonymous union/struct 只填到第一個 byte。
    - local aggregate init 的 brace-elision + string 路徑仍有錯（`lt/lu/lv/lt2/flow`）。
    - `contains_empty` / flexible array 尺寸與輸出長度仍與預期有差異（`ce/gw`）。

### 2) AArch64 `long double` / `va_arg` ABI 尚未支援完整
- Cases:
  - `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/va-arg-5.c`
  - `tests/llvm-test-suite/SingleSource/Regression/C/gcc-c-torture/execute/va-arg-6.c`
  - `tests/c-testsuite/tests/single-exec/00204.c`
- 目前失敗型態:
  - `va-arg-5.c` / `va-arg-6.c`: `RUNTIME_FAIL`
  - `00204.c`: `RUNTIME_MISMATCH`
- 現象:
  - Clang 可正常編譯並執行這些案例。
  - 我們產生的 IR 仍把 `long double` 降成 `double`：
    - `src/codegen/llvm/hir_to_llvm.cpp` 目前 `TB_LONGDOUBLE -> "double"`，`sizeof(TB_LONGDOUBLE) -> 8`
    - 但 Clang 在 AArch64 對這些案例使用 `fp128` / 16-byte `long double`
  - 因此 `va_arg(args, long double)`、`long double` variadic call lowering、以及 `struct { long double ... }` 的 HFA 傳參/取參都會錯。
  - `00204.c` 中所有 `long double` 與 HFA-long-double 區段目前都讀成 `0.0`，其餘 `float` / `double` 區段已可通過。
- 備註:
  - 這不是 LLVM `va_arg` IR 本身的限制；根因是目前後端尚未實作 AArch64 `long double` ABI 與 `fp128` lowering。
