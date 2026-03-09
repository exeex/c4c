# Sema / IR Split Migration Plan (Rebased)

Last updated: 2026-03-09

## 高優先級：背景殘留 testcase（卡資源）

來源：`/workspaces/c4c/build/leftover_testcase_kill_20260309T083735Z.log` 的殘留 `*.c2ll.bin` 行程。  
對應規則：去掉 `.c2ll.bin`，例如 `pr79354.c.c2ll.bin -> pr79354.c`。

- `20020413-1.c`
- `20020529-1.c`
- `20030909-1.c`
- `20050502-1.c`
- `20070517-1.c`
- `20090527-1.c`
- `921113-1.c`
- `950710-1.c`
- `981019-1.c`
- `990208-1.c`
- `990604-1.c`
- `991216-4.c`
- `loop-2b.c`
- `pr17078-1.c`
- `pr40668.c`
- `pr47538.c`
- `pr53465.c`
- `pr58277-2.c`
- `pr64260.c`
- `pr64756.c`
- `pr79354.c`
- `pr79737-1.c`
- `pr88714.c`
- `pr88739.c`
- `vrp-6.c`

## Unstash Review（2026-03-09）

### Findings

1. High: local enum constant value 洩漏到外層 scope / 後續函式  
   - 檔案：`src/frontend/sema/sema_validate.cpp`（`enum_const_vals_` 與 `bind_enum_constants_local`）  
   - 現象：`enum_const_vals_` 是 validator 級別 map，local enum 常數加入後不會在 `leave_scope()` 移除。  
   - 風險：`case` 常數折疊 `eval_int_const_expr(..., &enum_const_vals_)` 可能拿到過期值，造成 duplicate-case 偽陽性或錯誤常數判定。

### Action Plan（High Priority）

1. [Done] 將 enum 常數值改為 scope-aware 結構（與 `scopes_` 同步 push/pop），避免名稱污染。  
2. [Done] `eval_int_const_expr` 查值先走 local-scope，再 fallback global enum map。  
3. [Todo] 補最小回歸測試：  
   - 函式內 local enum 與 global enum 同名，不同值。  
   - 離開 block 後 `switch case` 仍使用正確外層 enum 值。

### Progress Update（2026-03-09）

- 已在 `src/frontend/sema/sema_validate.cpp` 實作：
  - `enum_const_vals_global_` + `enum_const_vals_scopes_` 雙層查找。
  - `enter_scope()/leave_scope()` 同步維護 local enum value scope。
  - `case` 常數折疊改用 scope-aware enum lookup。
- 驗證：
  - `cmake --build build -j4` ✅
  - `ctest --test-dir build -R tiny_c2ll_tests --output-on-failure` ✅

## Agent Handoff (Claude 接手用)

本檔主要給接手 agent，請直接照下面流程執行，不要先跑全量長測。

### 0) Build / Test 基本指令

```bash
rm -rf build
# configure（torture 預設 OFF；不要手動開）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

# build
cmake --build build -j4

# 核心 gate（已排除 llvm_gcc_c_torture label）
cmake --build build --target ctest_core -j4

# focused rerun
ctest -j --test-dir build -R tiny_c2ll_tests --output-on-failure
ctest -j --test-dir build -R '^ccc_review_' --output-on-failure
ctest -j --test-dir build -R '^negative_tests' --output-on-failure
ctest -j --test-dir build -R c_testsuite --output-on-failure
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

### 2.1) 本輪已修（2026-03-09）

以下 gcc torture case 已由 fail 轉為 pass：

- `llvm_gcc_c_torture_20000223_1_c`
  - 修正重點：`CallExpr` unresolved callee 視為 external symbol，並補上 external `declare`；避免 `call void 0(...)` 與 undefined callee。
  - 連帶修正：call 參數 coercion 依函式簽章對齊，避免 `float 0` 這類非法 IR 常數組合。
- `llvm_gcc_c_torture_20020619_1_c`
  - 修正重點：同上（`call void 0(...)` / missing external declaration）。
- `llvm_gcc_c_torture_20041124_1_c`
  - 修正重點：同上（`call void 0(...)` / missing external declaration）。
- `llvm_gcc_c_torture_20010924_1_c`
  - 修正重點：flexible-array 相關 global init 與 member decay 修正，避免非法 `getelementptr ptr, ...` 與 runtime segfault。

### 3) 目前核心失敗（2026-03-09）與簡短分析

`ctest_core` 目前 11 個失敗，分類如下：

1. `negative_tests_bad_flow_uninitialized_read`
   - 現象：negative test 預期編譯失敗，但目前編譯成功。
   - 分析：`sema_validate.cpp` 已移除「uninitialized local read」報錯，這和該 negative case 預期衝突。
   - 建議：二選一（需團隊決策）：
     - 回復此檢查並維持為 hard error（維持現有測試語意）
     - 或將該測試從 negative 移到 warning/positive 組

2. `c_testsuite_tests_single_exec_00050_c`
   - 現象：runtime exit=3。
   - 分析：switch/short-circuit/CFG 仍有語意差異；雖已修 label 未定義問題，但執行語意尚未對齊。
   - 位置：`ast_to_hir.cpp`（switch block/break target）、`hir_to_llvm.cpp`（switch dispatch）。

3. `c_testsuite_tests_single_exec_00078_c`
   - 現象：`call i32 %t1(...)`，callee 型別被當成 `i32` 非 `ptr`。
   - 分析：函式指標 call lowering 仍有 type resolve 漏洞（與 `00040` 類似但覆蓋不足）。

4. `c_testsuite_tests_single_exec_00124_c`
   - 現象：`load void, ptr`（非法 LLVM IR）。
   - 分析：rvalue/type 推導 fallback 到 `void` 時，load path 未阻擋。

5. `c_testsuite_tests_single_exec_00130_c`
   - 現象：`getelementptr ptr, ptr %lv.p, i64 0, i64 0`（indices 非法）。
   - 分析：陣列/指標 decay 與 GEP element type 不一致（目前有多處用 `ptr` 當 element type）。

6. `c_testsuite_tests_single_exec_00149_c`, `00150_c`
   - 現象：global constant initializer type mismatch。
   - 分析：aggregate/global initializer 組裝出現型別不一致（struct/ptr/array 混用）。

7. `c_testsuite_tests_single_exec_00151_c`
   - 現象：runtime segfault。
   - 分析：高機率是記憶體位址計算或 aggregate 初始化錯誤導致（與 00130/00149/00150 可能同根）。

8. `c_testsuite_tests_single_exec_00143_c`, `00209_c`
   - 現象：runtime wrong result / IR parse 失敗（`expected value token`）。
   - 分析：屬於 expression emission 或 terminator/空值發射的健壯性缺口。

9. `c_testsuite_tests_single_exec_00204_c`
   - 現象：frontend 報 `assignment to const-qualified lvalue`。
   - 分析：sema const 規則比 c-testsuite 預期更嚴；可能是 false positive 或規則過度擴張。

### 4) 修復優先序（更新）

在原本 `00051 -> 00040 -> 00050` 順序後，追加以下序列：

1. `00050`（先收斂控制流語意，避免後續 debug 噪音）
2. `00078`（函式指標 call type）
3. `00124`（禁止 `load void`）
4. `00130`（GEP element type 一致性）
5. `00149`, `00150`（global aggregate init typing）
6. `00151`（runtime crash，通常隨上面幾項一併收斂）
7. `00143`, `00209`（expression/IR emission 健壯性）
8. `00204`（const 規則與測試契約對齊）
9. `negative_tests_bad_flow_uninitialized_read`（需先決策語意，再落地）

### 5) 下一步主要工作：GCC Torture 回圈

後續主流程改為以下 5 步，反覆執行直到收斂：

1. `ctest gcc_torture`
   - 先開啟 torture 註冊（若尚未開）：
     ```bash
     cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_LLVM_GCC_C_TORTURE_TESTS=ON
     cmake --build build -j4
     ```
   - 執行 torture：
     ```bash
     ctest --test-dir build -R llvm_gcc_c_torture --output-on-failure
     ```

2. `clean dead process`
   - 先清理殘留 `*.bin`：
     ```bash
     pkill -9 -f '^/workspaces/c4c/build/llvm_gcc_c_torture/.*\.bin($| )'
     ```
   - 再清孤立進程（PPID=1）：
     ```bash
     ps -eo pid=,ppid=,cmd= | awk '$2==1 && $0 ~ /^ *[0-9]+ +1 +\\/workspaces\\/c4c\\/build\\/llvm_gcc_c_torture\\/.*\\.bin( |$)/ {print $1}' | xargs -r kill -9
     ```

3. `analysis test report`
   - 收斂失敗類型（frontend fail / backend fail / runtime nonzero / timeout）。
   - 以單一根因群組分類，不要逐案散修。

4. `select a testcase to fix`
   - 每輪只挑 1 個代表性 testcase 修。
   - 先做最小重現，再做精準修復，最後只 rerun 同群案例。

5. `update plan.md && commit`
   - 每輪固定更新：
     - 新失敗摘要
     - 本輪處理 testcase
     - 修復結果與下一輪目標
   - 然後 commit（訊息需反映 testcase 與修復主題）。

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
