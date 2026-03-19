# c4c Next-Step Plan

## Goal

從 `ideas/*plan.md` 中挑選幾個目前最有價值、而且彼此能串成合理路線的任務，
整理成一份新的總計畫。這份計畫優先考慮：

- 能直接擴大 C++ 可用能力的項目
- 能降低後續實作成本與除錯成本的項目
- 已完成項目不重複納入主線

## Chosen Tasks

本計畫選擇以下來源的任務：

- `ideas/namespace_plan.md`
- `ideas/clangtest_plan.md`
- `ideas/iterator_plan.md`
- `ideas/container_plan.md`
- `ideas/lir_split_plan.md`

未納入主線的項目：

- `ideas/rvalue_ref_plan.md`
- `ideas/operator_overload_plan.md`
- `ideas/range_for_plan.md`

原因：這三項在原始文件中已標示為目前里程碑內完成，適合作為依賴前提，不再列為新的主任務。

## Priority Order

### 1. Namespace support refactor

來源：`ideas/namespace_plan.md`

這是最值得優先處理的語言基礎設施之一。namespace 如果仍主要依賴字串攤平，
後續在名稱查找、匿名 namespace、符號命名與標頭可擴充性上都會持續付出成本。

本階段目標：

- 引入明確的 namespace context 物件
- 讓 qualified name 保留結構，而不是只保留字串
- 讓 declaration 直接記錄所屬 context
- 把 lookup 從字串重寫逐步搬到 context-based model

完成標準：

- `::a::b` 與 `a::b` 可被正確區分
- reopen namespace 與 anonymous namespace 有穩定語意
- runtime/frontend 測試可驗證 namespace lookup，而不只是 parse-only

### 2. Lightweight error recovery + negative test verify

來源：`ideas/clangtest_plan.md`

這個任務的價值很高，因為它會直接改善開發效率、診斷品質與外部測試的可用性。
一旦能穩定地在單一檔案中回報多個有效錯誤，之後做 namespace、iterator 這類功能時，
回歸成本會明顯下降。

本階段目標：

- 固定 diagnostics 格式為穩定的 `file:line:column: error: ...`
- 加入 hard error limit
- 加入最小 `InvalidExpr` / `InvalidType`
- 補 top-level、statement、paren-list 的同步恢復
- 讓測試 runner 支援最小 `expected-error` 子集

完成標準：

- 一個壞 declaration 不會毀掉整個 TU
- cascade errors 顯著下降
- 可跑一小批 curated Clang-style negative tests in `verify` mode

### 3. Finish iterator/container usability slice

來源：`ideas/iterator_plan.md` + `ideas/container_plan.md`

operator overload 與 range-for 已完成，現在最值得做的是把 iterator 與小型 container
這條使用路徑補成一條完整、可驗證的能力線。這能把先前的語言支援轉成真正的可用案例。

本階段目標：

- 完成第一個可工作的 custom iterator slice
- 支援 container 的 `begin()` / `end()`
- 驗證 manual iterator loop
- 補一個固定容量 `FixedVec` smoke test
- 確認 `size()`, `empty()`, `data()`, `front()`, `back()`, `operator[]`,
  `push_back()`, `pop_back()`, `begin()`, `end()`, `range-for` 可一起工作

建議切法：

- 先做 iterator Phase 0-3
- 再跑 `fixed_vec_smoke.cpp` 作為整合驗證
- 只在 smoke test 真正卡住時補最小必要修正

完成標準：

- 自訂 iterator 可在一般 loop 中穩定運作
- 小型 fixed-storage container 可通過整合 smoke test
- 不需要再加 iterator 專用 hack 來支撐 range-for

### 4. HIR -> LIR split

來源：`ideas/lir_split_plan.md`

這是架構價值最高的重構項，但不適合排在最前面。等前面幾個語言/測試面向更穩定後，
再做這個分層，風險和返工都會比較低。

本階段目標：

- 把目前 `hir_emitter` 的 lowering 與 LLVM text emission 分離
- 建立最小可檢視的 `LirModule`
- 讓 `llvm_codegen.cpp` 轉為 `HIR -> LIR -> LLVM printer`

建議階段：

- Stage 0: skeleton 與最小 `LirModule`
- Stage 1: 先把字串 sink 換成結構化 LIR
- Stage 2: 抽出 `LirPrinter`
- Stage 3: 再逐步把特殊 case 正規化成 LIR ops

完成標準：

- 行為維持相容
- printer 不再承擔語意修補責任
- 後續 backend work 有清楚邊界可依附

## Execution Roadmap

### Milestone A: Frontend correctness foundation

1. 做 namespace context refactor 的 Phase 1-4
2. 同步補 parse/runtime tests
3. 避免再擴張字串攤平 namespace 模型

### Milestone B: Diagnostics and regression quality

1. 落地 minimal error recovery
2. 加入 `expected-error` 驗證能力
3. 建立小型 curated negative-test allowlist

### Milestone C: User-visible container usability

1. 完成 iterator 最小可用面
2. 補 container `begin()/end()` 與 manual loop 測試
3. 以 `fixed_vec_smoke.cpp` 做整合驗證

### Milestone D: Codegen architecture cleanup

1. 建立 LIR 邊界
2. 將 lowering 與 printer 分離
3. 保持 LLVM IR 行為相容，避免一次性重寫

## Why This Order

- Namespace 是真實 C++ 程式可擴展性的基礎，越晚修越容易讓 hack 滲透到更多層。
- Error recovery 能立即提高之後每個功能開發的回饋品質，投資報酬率很高。
- Iterator/container slice 能把已完成的 operator/range-for 支援轉化為具體成果。
- HIR/LIR split 很重要，但屬於在功能面較穩後再做的架構整理，順序放後面更穩。

## Definition Of Success

這份新計畫完成時，專案應該具備以下特徵：

- namespace 不再主要依賴字串攤平
- 編譯器能在壞輸入下回報多個穩定且有用的錯誤
- 小型 iterator/container 範例可作為端到端能力證明
- codegen 內部具有清楚的 HIR/LIR/LLVM printer 邊界
