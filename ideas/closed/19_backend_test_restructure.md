# Backend Test Restructure

Status: Closed
Completed: 2026-03-31

## Completion Notes

- helper split complete: `backend_test_util.hpp` (assertions/utilities) + `backend_test_fixtures.hpp` (module builders); `backend_test_helper.hpp` reduced to thin aggregator
- argv[1] filter complete: `test_filter()` + updated `RUN_TEST` macro; all 5 test `main()` functions accept the argument
- file-by-category split (item 3) deferred: aarch64 (2708 lines) and x86 (2566 lines) files share file-local `make_*` helpers across all categories; a clean split would require moving helpers to yet another shared header — deferred to a future idea
- parameterized merge (item 4) remains long-term
- Regression: 2534/2671 before and after (no change)

## Problem
tests/backend/ 目前是一萬行勉強拆成五個檔案，內部沒有分層。

## Proposed Changes (priority order)

### 1. 拆 helper（utility vs fixture）
- `backend_test_util.hpp` — 斷言 (`fail`, `expect_*`, `RUN_TEST`)、路徑、shell 工具
- `backend_test_fixtures.hpp` — module builder 函式 (`make_return_add_module` 等)
- 成本低，收益大

### 2. 加 argv[1] filter
- main() 裡用 argv[1] substring match test name，十行程式碼
- 能單跑特定 test，大幅改善 debug 體驗

### 3. 按功能拆 aarch64/x86 test files
- `*_rendering_tests.cpp` — emit_module asm 輸出驗證
- `*_assembler_tests.cpp` — parser / encoder / ELF writer
- `*_linker_tests.cpp` — linker slice 與 executable 驗證
- 失敗模式完全不同，不該塞同一個 binary

### 4. aarch64/x86 parameterized 合併（長期）
- scaffold、compare-and-branch、direct call、regalloc 系列邏輯相同
- 只是 target 和預期 asm string 不同
- 抽成 parameterized test 共用同一套 test function

## Execution Notes
- Dirty work（大量機械性搬動 + 驗證不破壞）適合拆 subagent 用小模型並行跑
- 每個 subagent 負責一個拆分維度，互不干擾
