# Codegen Refactor Plan

## Goal

把目前名義上的 HIR -> LIR split，落實成真正的兩段式 pipeline：

`HIR -> HIRToLIR -> LIR -> LLVM emitter/printer`

最終目標不是停在「產生 `.ll` 字串」，而是建立一條之後可以：

- 直接呼叫 LLVM / libclang / LLVM C++ API 來 emit IR，
- 用 flag 在新舊 codegen 間切換，
- 在重構期間持續保住現有測試，
- 用新舊雙路輸出快速比對差異。


## Reality Check

目前 repo 的狀態不是「LIR split 已完成」，而是：

- `src/codegen/lir/hir_to_lir.cpp` 仍是 stub
- `src/codegen/llvm/hir_emitter.cpp` 仍然持有真正的 lowering 邏輯
- `src/codegen/lir/lir_printer.cpp` 已存在，但目前吃到的 `LirModule`
  仍主要是由 `HirEmitter` 直接組出來

換句話說，現在完成的是：

- LIR data structures 已建立
- printer 已抽出

但**尚未完成**的是：

- 將 lowering responsibility 從 `hir_emitter.cpp` 搬到 `hir_to_lir.cpp`


## Main Objective

這份計畫只聚焦一條主線：

1. 真正完成 `hir_emitter.cpp -> hir_to_lir.cpp` 的責任搬移
2. 保留舊路徑，讓新舊可並行
3. 加入快速 diff 驗證模式
4. 為後續「不經 `.ll` 字串、直接透過 LLVM API emit IR」鋪路


## Non-Goals

這份計畫不做：

- 一次性刪掉舊的 `HirEmitter`
- 同一波直接導入完整 LLVM IRBuilder backend
- 同時處理 unrelated frontend feature work
- 把 `.ll` printer 全部重寫成另一種 IR 格式

原則是：

- 先把 lowering 邊界搬正
- 再讓新舊並行
- 最後才考慮把 LLVM API emitter 接上


## Architectural Decision

### Preferred path

主程式先維持單一入口，靠 flag 切換 codegen 路徑。

建議加入：

- `--codegen=legacy`
- `--codegen=lir`
- `--codegen=compare`

其中：

- `legacy`: 走現有 `HirEmitter`
- `lir`: 走 `hir_to_lir` + LLVM printer
- `compare`: 同一個輸入跑兩次，輸出兩份 `.ll` 並比較差異

### Optional support tool

若 CLI/測試隔離上比較方便，可以補第二個 app：

- `src/apps/c4clle2e.cpp`

用途不是永久取代 `c4cll`，而是：

- 專門跑新路徑
- 專門做新舊 diff 驗證
- 避免主 CLI 在過渡期堆太多 debug 旗標

### Decision rule

優先順序建議：

1. 先做 `--codegen=` flag
2. 若 compare/debug 流程太雜，再補 `c4clle2e`


## Deliverables

這個 refactor 結束時，至少要有：

- 真正有內容的 `src/codegen/lir/hir_to_lir.cpp`
- 舊 `src/codegen/llvm/hir_emitter.cpp` 只保留 legacy path 所需邏輯
- `llvm_codegen.cpp` 可按 flag 選擇新舊路徑
- 可自動產出 legacy/new 兩份 `.ll`
- 可直接比較新舊 `.ll` 差異
- 針對 compare mode 的回歸測試
- 一條在主線後自動接續的 `std::vector` bring-up 任務，
  具體定義於 `ideas/std_vector_bringup_plan.md`


## Concrete Split Boundary

### What must move out of `hir_emitter.cpp`

以下責任應逐步搬到 `src/codegen/lir/hir_to_lir.cpp`：

- `FnCtx` 型 lowering state
- block / label 建立
- temp / SSA value naming state
- alloca hoisting decision
- lvalue lowering
- rvalue lowering
- statement lowering
- CFG terminator construction
- string pool / extern decl / intrinsic requirement tracking
- function / global lowering into `LirModule`

### What can stay printer-side

以下責任應留在 printer / LLVM-specific rendering：

- LLVM textual syntax
- `%` / `@` / label naming policy
- `declare` / `define` 文本格式
- LLVM intrinsic spelling
- target triple / datalayout 的最終文本輸出

### What legacy `HirEmitter` should become

重構後 `HirEmitter` 不應再是唯一 lowering owner。

它應該變成二擇一：

1. legacy backend wrapper
2. 或完全保留不動，僅作為 `--codegen=legacy` 路徑

但不應再繼續承擔新 LIR 路徑的主實作。


## Migration Strategy

## Follow-on Track

主線完成第一個安全落點後，agent 應自動接續下一條 bring-up 線：

- `ideas/std_vector_bringup_plan.md`

自動接續條件：

1. `--codegen=legacy|lir|compare` 已存在
2. compare mode 已能產生雙 `.ll`
3. 至少一組 compare-mode smoke test 已落地

一旦以上三項完成，下一個預設工作不是再抽象討論 LIR，
而是直接推進：

- `tests/cpp/std/std_vector_simple.cpp`

這樣做的目的，是讓 codegen refactor 完成第一段後，立刻有一個真實、
系統標頭密集、且能同時驗證新舊路徑的 end-to-end 目標。

## Phase 0: Freeze The Boundary

### Objective

先明確定義「什麼叫完成 HIR -> LIR split」。

### Tasks

- 在文件中明寫：`hir_to_lir.cpp` 不是 skeleton，而是 lowering owner
- 列出 `hir_emitter.cpp` 中必搬的函式群
- 標記哪些函式屬於 lowering，哪些屬於 printing

### Exit Criteria

- 不再用「已有 LIR struct」當作 split 完成的判準


## Phase 1: Add Parallel Codegen Selection

### Objective

讓新舊路徑可以同時存在，避免重構一次打壞全部。

### Tasks

1. 在 CLI 加入 codegen path 選項

- `--codegen=legacy`
- `--codegen=lir`
- `--codegen=compare`

2. 在 `llvm_codegen.cpp` 加入口由

- legacy path: 現有 `HirEmitter`
- lir path: `hir_to_lir` + `lir_printer`

3. 在 compare mode 支援雙輸出

- 產出 `legacy.ll`
- 產出 `lir.ll`
- 回報 diff 結果

### Exit Criteria

- 新舊路徑都能從同一個 CLI 執行
- compare mode 可做 smoke verification
- 完成後，預設接續 `ideas/std_vector_bringup_plan.md` 的 Phase A


## Phase 2: Make `hir_to_lir.cpp` Own Module Lowering

### Objective

先搬最外層骨架，不先碰所有 expression 細節。

### Tasks

1. 搬 module-level lowering

- globals
- function enumeration / dedup
- extern decl tracking
- string pool ownership
- intrinsic requirement flags

2. 把 `LirModule` 建構邏輯從 `HirEmitter::emit()` 移到 `lower(...)`

3. 保持 printer 完全吃 `LirModule`

### Exit Criteria

- `hir_to_lir.cpp` 不再回傳空 module
- `HirEmitter::emit()` 不再自己組完整 `LirModule`


## Phase 3: Move Function Skeleton Lowering

### Objective

把 function/block/terminator 架構先搬過去，再搬 expression 細節。

### Tasks

1. 搬 `FnCtx` 到 LIR lowering side
2. 搬 block creation / label creation
3. 搬 terminator emission

- branch
- cond branch
- return
- switch
- indirectbr

4. 搬 alloca hoisting

### Exit Criteria

- `hir_to_lir.cpp` 能產生完整 block graph
- function 結構不再由 `hir_emitter.cpp` 主導


## Phase 4: Move Statement / Expression Lowering In Slices

### Objective

按風險分組，把真正的 lowering 一塊一塊搬走。

### Recommended slice order

1. simple expressions

- literals
- decl refs
- casts
- basic arithmetic / compare

2. storage and addressability

- load/store
- lvalue lowering
- member/index gep

3. control-flow-bearing expressions/statements

- assign
- ternary
- logical short-circuit
- if/while/for/do-while

4. ABI-heavy and special cases

- calls
- va_arg
- memcpy / va_start / va_end / va_copy
- stacksave / stackrestore
- bitfield ops
- inline asm

### Exit Criteria

- 對每個 slice，都有 legacy vs lir compare 測試
- 每搬完一組，就把 `hir_emitter.cpp` 對應邏輯標成 legacy-only


## Phase 5: Add Fast Regression Guard

### Objective

讓重構可以用差異比對快速守住行為。

### Tasks

1. compare mode 產生兩份 `.ll`
2. 提供 deterministic diff

- 先做文字 diff
- 必要時加入 normalize pass

3. 建立小型 allowlist

- smoke cases
- iterator/container cases
- namespace-heavy cases
- varargs / bitfield / switch / indirectbr cases

### Exit Criteria

- 每次搬移都能快速回答「新舊輸出差在哪」
- 有差異時可快速定位是 printer 還是 lowering
- 此階段的第一個後續 consumer 應是 `tests/cpp/std/std_vector_simple.cpp`


## Phase 6: Prepare For LLVM API Emission

### Objective

在 LIR 路徑穩定後，為未來直接呼叫 LLVM API emit IR 鋪路。

### Tasks

1. 將 `LirModule` 的資訊需求補齊到足以支撐 API emitter
2. 定義新的 backend 介面

- `emit_llvm_text_from_lir(...)`
- `emit_llvm_module_from_lir(...)`

3. 保持 text printer 與 API emitter 共享同一份 LIR

### Exit Criteria

- `.ll` printer 不再是唯一 backend consumer
- 未來可新增 LLVM C++ API emitter，而不需要再回頭重拆 lowering


## Success Criteria

這份計畫完成時，應該能明確做到：

- `src/codegen/lir/hir_to_lir.cpp` 不是 stub，而是主要 lowering owner
- `src/codegen/llvm/hir_emitter.cpp` 不再是假名上的 legacy、實際上的主 lowering
- 新舊 codegen 可以靠 flag 並行
- 可以一鍵產生新舊 `.ll` 並看 diff
- 現有測試能在重構中持續守住
- 前述基礎完成後，agent 會自動轉去執行 `ideas/std_vector_bringup_plan.md`
  推進 `tests/cpp/std/std_vector_simple.cpp`
- 後續導入 LLVM API emitter 時，不需要再先重做一次 HIR/LIR split
