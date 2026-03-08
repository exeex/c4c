# Sema / IR Split Migration Plan (Rebased)

Last updated: 2026-03-08

## Agent Handoff (Claude 接手用)

本檔主要給接手 agent，請直接照下面流程執行，不要先跑全量長測。

### 0) Build / Test 基本指令

```bash
# configure（torture 預設 OFF；不要手動開）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# build
cmake --build build -j4

# 核心 gate（已排除 llvm_gcc_c_torture label）
cmake --build build --target ctest_core -j4

# focused rerun
ctest --test-dir build -R tiny_c2ll_tests --output-on-failure
ctest --test-dir build -R '^ccc_review_' --output-on-failure
ctest --test-dir build -R '^negative_tests' --output-on-failure
ctest --test-dir build -R c_testsuite --output-on-failure
```

### 1) 避免殘留無窮迴圈程序

若發現 `build/llvm_gcc_c_torture/*` 或異常久跑 binary：

```bash
ps -eo pid,ppid,etime,cmd | rg '/workspaces/c4c/build/llvm_gcc_c_torture|c2ll.bin|clang.bin'
kill -9 <pid...>
```

### 2) 下一個修復順序（照順序做）

1. `c_testsuite_tests_single_exec_00051_c`
   - 現象：switch 生成 undefined label（`%sw.end.*`）  
   - 目標：修 switch lowering label/edge，確保無未定義 label，case/default 跳轉正確。
   - 驗收：
     ```bash
     ctest --test-dir build -R c_testsuite_tests_single_exec_00051_c --output-on-failure
     ```

2. `c_testsuite_tests_single_exec_00040_c`
   - 現象：`call void 0(...)`（callee 解析錯成常數）  
   - 目標：修 call lowering/callee type 推導，確保函式指標與函式名都走正確 call path。
   - 驗收：
     ```bash
     ctest --test-dir build -R c_testsuite_tests_single_exec_00040_c --output-on-failure
     ```

3. `c_testsuite_tests_single_exec_00050_c`
   - 現象：IR 可編譯但 runtime 結果錯（控制流/短路語意）  
   - 目標：修 CFG 或 short-circuit 分支語意錯接。
   - 驗收：
     ```bash
     ctest --test-dir build -R c_testsuite_tests_single_exec_00050_c --output-on-failure
     ```

4. 三個都綠後，回歸這組：
   ```bash
   ctest --test-dir build -R 'c_testsuite_tests_single_exec_(00031|00032|00033|00037|00039|00040|00045|00049|00050|00051)_c' --output-on-failure
   ```

5. 再跑核心 gate：
   ```bash
   cmake --build build --target ctest_core -j4
   ```

## Goal

Refactor pipeline from:

- `Parser -> IRBuilder (semantic + LLVM lowering mixed)`

to:

- `Parser -> Sema (validation + typed info) -> HIR -> Backend`

with these hard rules:

- `next` pipeline must not depend on legacy semantic behavior.
- `codegen` must not own semantic rejection logic.
- legacy path stays available only as comparison/fallback binary during migration.

## Why Rebase Now

Current state is effectively "Phase 3 half-done":

- HIR data model and AST->HIR lowering exist.
- Native HIR->LLVM emitter exists for a subset.
- But semantic rejection still depends on behavior historically living in legacy IRBuilder.

This causes architectural drift: failures in `next` are currently dominated by missing sema checks, not just missing codegen coverage.

## Target Architecture

```txt
parse
  -> sema_validate (hard errors + diagnostics)
  -> lower_ast_to_hir
  -> hir_to_llvm_native
  -> (optional) future hir_to_dag / native backend
```

Legacy components are allowed only for:

- parity diff tooling
- emergency debug path (`--pipeline=legacy`)

They are not allowed as required runtime dependencies of `--pipeline=hir`.

## Phase Plan

### Phase 0: Parallel Entry (completed)

- `tiny-c2ll-next` binary introduced.
- legacy binary remains test default during migration.

### Phase 1: Sema Helper Extraction (completed)

- `sema_driver` helper utilities extracted from IRBuilder.
- No behavior change target achieved.

### Phase 2: Minimal HIR (completed)

- `ir.hpp`, `ast_to_hir`, HIR summary/dump integrated.

### Phase 3A: Independent Sema Validation Gate (now)

Deliverables:

- Add explicit sema validation module in `src/frontend/sema/` (new pass API).
- `tiny-c2ll-next --pipeline=hir` flow becomes:
  - parse -> sema_validate -> lower_hir -> emit_native
- Remove any mandatory call path to legacy IRBuilder from `pipeline=hir`.

Initial validation scope (must block compile with non-zero exit):

- control-flow misuse: `break/continue/case/default` outside valid context
- symbol resolution: undeclared/undefined usage
- call arity mismatch for known prototypes
- const correctness:
  - assignment to const lvalue
  - passing const pointer to mutable parameter
  - dropping const in implicit pointer assignment
  - const object inc/dec
- invalid casts:
  - cast to incomplete struct/union object type
  - cast to unknown type name

Acceptance:

- All `negative_tests_*` pass under `tiny-c2ll-next` without legacy dependency.
- Failing sema diagnostics come from sema module, not codegen exceptions.

### Phase 3B: Native HIR->LLVM Coverage Expansion

Deliverables:

- Complete native emission for currently incomplete high-impact features:
  - struct/union member access
  - string/global initializers
  - switch/case dispatch completion
  - variadic call lowering path
- Keep sema and codegen responsibilities separated.

Acceptance:

- `tiny_c2ll_tests`, `ccc_review_*`, `c_testsuite_*` converge on parity for agreed subset.
- No semantic regressions introduced by codegen-only changes.

### Phase 3C: Bridge Retirement in `next`

Deliverables:

- Remove runtime dependency on bridge in default `next` path.
- Keep legacy invocation only in explicit comparison mode/tooling.

Acceptance:

- `--pipeline=hir` is self-contained.
- CI gate for non-torture suites no longer requires legacy bridge.

### Phase 4: DAG Layer Activation

Deliverables:

- Add `hir_to_dag` lowering skeleton + `--dump-dag` output.
- Keep LLVM backend functional while DAG grows.

Acceptance:

- representative files produce inspectable DAG output.

### Phase 5: Cutover Prep

Deliverables:

- Stabilize `next` on non-torture suites.
- Add parity diff tooling (legacy vs next outputs/diagnostics).

Acceptance:

- agreed suite parity and documented perf/memory baseline.

### Phase 6: Default Switch + Legacy Decommission

Deliverables:

- switch default compiler path to `next`
- remove legacy monolith after sustained green runs

Acceptance:

- team sign-off and transition window complete.

## Test Strategy (Rebased)

Execution order (mandatory):

1. Run aggregate gate first:
   - `cmake --build build --target ctest_core`
2. If `ctest_core` fails, fix in this practical priority:
   - `tiny_c2ll_tests` first
   - then `ccc_review_*`
   - then `c_testsuite_*`

Within `tiny_c2ll_tests`, `negative_tests` remain the first-fix subset
(semantic correctness gate).

Core aggregate gate:

- `cmake --build build --target ctest_core`

Focused rerun commands:

- `ctest -R tiny_c2ll_tests`
- `ctest -R '^negative_tests'`
- `ctest -R ccc_review`
- `ctest -R c_testsuite`

Exclusion policy in migration loop:

- `llvm_gcc_c_torture_*` excluded from default gate until Phase 5+
- keep torture test registration OFF by default in CMake
  (`-DENABLE_LLVM_GCC_C_TORTURE_TESTS=ON` only for explicit runs)

Timeout policy:

- default per-test timeout: 30s
- any >30s test runtime treated as suspicious regression unless explicitly documented

## Current Status Snapshot (2026-03-08)

- Phase 0/1/2: done.
- Phase 3: partially done.
  - native HIR emitter exists but sema validation is incomplete.
  - major failure mode is semantic rejection gaps, not parser stability.
- `next` architecture correction required now:
  - move hard semantic checks into sema pass before codegen.

### Latest Progress Update (2026-03-08)

- Stabilized a large subset of early failing cases:
  - `tiny_c2ll_tests`: green
  - `ccc_review_*`: all green
  - `c_testsuite` early band now includes greens for:
    `00004`, `00005`, `00016`, `00018`, `00020`, `00025`,
    `00031`, `00032`, `00033`, `00034`, `00037`, `00039`, `00045`, `00049`,
    `00054`, `00055`, `00057`, `00058`, `00072`, `00073`
- Fixed timeout/infinite-loop class bug in control-flow lowering:
  - `c_testsuite_tests_single_exec_00034_c` now passes (previously timeout)
- Safety action:
  - cleaned leftover runaway processes under `build/llvm_gcc_c_torture/*`
  - switched default build behavior to not register torture tests unless explicitly enabled

## Immediate Next 3 PRs

1. PR-A: Introduce `sema_validate` pass and wire it into `tiny-c2ll-next` before HIR/codegen.
2. PR-B: Implement first-wave hard checks (negative suite core: control-flow misuse + const/cast/call arity).
3. PR-C: Remove mandatory legacy bridge calls from `--pipeline=hir`; keep bridge behind explicit legacy mode only.

## Priority Queue (Post-Timeout Fix)

Based on current `ctest_core` snapshot (timeouts removed, many fail-fast cases remain),
execute in this order:

1. P0: Stabilize frontend sema false positives in `c_testsuite` first-fail band (`00004`-`00058`).
   - Focus symptoms:
     - undeclared identifier reports for valid scoped names
     - over-strict const/arity diagnostics on accepted C patterns
   - Acceptance:
     - convert these from `FRONTEND_FAIL` to either pass or backend-diagnosable failures.

2. P1: Fix native LLVM type emission invariants (high-churn backend failures).
   - Focus symptoms:
     - invalid IR such as `zext ... to void`, `load void`, illegal cast opcode combos.
   - Acceptance:
     - no clang parse/type errors on `00004`-`00058` subset.

3. P2: Repair control-flow/block edge correctness beyond timeout set.
   - Focus symptoms:
     - wrong fallthrough/edge wiring causing incorrect runtime result (non-timeout).
   - Acceptance:
     - `c_testsuite_tests_single_exec_00041_c`-style logic cases stay green while expanding subset.

4. P3: Restore `ccc_review_*` literal/lexer compatibility.
   - Targets:
     - `0001_binary_literal`
     - `0002_hex_float_literal`
     - `0003_dot_float_literal`
     - `0004_unicode_escape_string`
     - `0005_unicode_escape_char`
     - `0006_dollar_identifier`
     - `0008_octal_literal`
   - Acceptance:
     - all `ccc_review_*` green under `tiny-c2ll-next`.

5. P4: Expand c-testsuite frontier incrementally.
   - Suggested marching windows:
     - batch A: `00004`-`00058`
     - batch B: `00072`-`00096`
   - Rule:
     - do not expand to next window until current window has no timeout and no frontend false positives.

## Guardrails

- No new semantic checks in `hir_to_llvm.cpp` unless they are IR structural invariants.
- Semantic user-facing diagnostics must originate from sema module.
- Any temporary fallback to legacy must be feature-flagged, off by default, and tracked with removal issue.
