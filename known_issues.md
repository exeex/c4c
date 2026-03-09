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

### 2) `c_testsuite_tests_single_exec_00204_c`（ID 315）
- Case: `tests/c-testsuite/tests/single-exec/00204.c`
- 失敗型態: `BACKEND_FAIL`（clang codegen crash）
- 現象:
  - AArch64 codegen 階段崩潰：`Running pass 'RegBankSelect' on function '@myprintf'`
  - `clang: error: unable to execute command: Segmentation fault`
- 推測:
  - 這支測試包含大量 AArch64 ABI / `va_arg` 結構傳參壓力，當前生成的 IR 觸發 clang backend crash（可能為前端輸出 IR 的型別/調用約定不一致）。
