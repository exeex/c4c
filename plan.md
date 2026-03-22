# Frontend Priority Plan

## Goal

把目前的主線優先順序重新排好，先解 operator overloading 相關的
frontend 問題，讓 EASTL / `std::vector` bring-up 不再反覆被同一類
parser/sema 缺口卡住。

這份計畫現在的主線不是 codegen refactor。
codegen 仍然重要，但暫時降為次要軌。

## New Priority Order

1. operator overloading and cast-like operator normalization
2. out-of-class method semantics needed by operator-heavy headers
3. EASTL / EABase / `std::vector` bring-up
4. deferred codegen refactor follow-up

## Why Reprioritize

最近的 `int128.h` / EASTL bring-up 已經證明：

- 目前最先爆開的不是 backend
- 而是 C++ operator-related frontend forms
- 尤其是 conversion operator、qualified operator definitions、
  out-of-class method parsing與語意銜接

如果這條不先補齊，後面無論追 `std::vector`、EASTL、甚至更多 C++
header，都会持續在同一類語法/語意坑重複卡住。

## Main Track: Operator Overloading

主計畫文件：

- `ideas/operator_overload_plan.md`

這條線的目標不是追求完整 C++ parity，而是先把目前實際會卡住
EASTL / STL-like code 的 operator forms 穩定下來。

### Current focus

- template conversion operator support
- out-of-class conversion operator stability
- implicit member lookup in out-of-class method bodies
- unqualified static member call resolution/lowering in out-of-class methods
- `operator new/delete/new[]/delete[]` declarator and explicit-call support
- `new` / placement-`new` / `delete` / `delete[]` expressions routed through
  operator lookup instead of stdlib assumptions
- cast-like operators在 HIR 中的 canonical form
  - `CastTo<T>(obj)`
  - `CastFrom<T>(args...)`

### Immediate success condition

以下條件成立時，operator 這條主線可以視為進入下一階段：

- `operator T()` 與 template conversion operator 都能穩定 parse
- out-of-class operator/method bodies 不再把後續 top-level parse 帶歪
- out-of-class method body 的 member/static-member 語意不再需要 testcase
  避開 unqualified spelling
- `operator new/delete/new[]/delete[]` 的 global/class-scope 宣告與顯式呼叫
  都穩定 parse
- EASTL / EABase 不再先撞上 operator-specific parser failure
  或 placement-`new` / delete-expression 缺口

## Follow-on Track: EASTL / std::vector Bring-up

主計畫文件：

- `ideas/std_vector_bringup_plan.md`

這條線現在不再是最前面第一順位，而是緊跟在 operator 主線之後。

### Rule

一旦 operator 主線把目前最前面的 blocker 清掉，就立刻回到：

- `tests/cpp/eastl/std_vector_simple.cpp`

目標不是抽象地說「operator 支援比較完整了」，
而是用實際 header-heavy case 驗證：

- 新 parser/sema 修正是否真的能推進系統/library headers
- 下一個 blocker 是不是 operator 以外的新類型問題

## Deferred Track: Codegen Refactor

原本的 codegen 主線先降為 deferred。

相關文件仍保留：

- `plan_todo.md`
- `ideas/lir_split_plan.md`

這代表：

- 已完成的 codegen 進度不丟
- 但接下來的預設工作不再是持續切 LIR lowering ownership
- 除非 operator / EASTL bring-up 暫時進入穩定等待，才回頭續推

## Concrete Next Actions

接下來的預設順序：

1. 先把 `operator new/delete/new[]/delete[]` 這組 operator family 收尾
   到 expression/lookup 層
2. 補 `new T` / `new T(args)` / `::new (p) T(args)` / `delete p` /
   `delete[] p` 的 parser + lowering
3. 接上 class-specific 與 global `operator new/delete` lookup
4. 先把已經掛進 `ctest -j` 的 new/delete 目標測試轉綠：
   - `cpp_positive_sema_new_expr_basic_parse_cpp`
   - `cpp_positive_sema_placement_new_expr_parse_cpp`
   - `cpp_positive_sema_class_specific_new_delete_parse_cpp`
   - `cpp_positive_sema_new_delete_side_effect_runtime_cpp`
   - `cpp_positive_sema_class_specific_new_delete_side_effect_runtime_cpp`
   - `cpp_positive_sema_new_array_delete_array_side_effect_runtime_cpp`
5. 回頭重跑 `tests/cpp/eastl/std_vector_simple.cpp`
6. 若 operator/new-delete 線已清到下一層，再回到 template conversion
   operator 與其他 frontend 子問題
7. 根據新的最早 blocker 決定是否繼續 operator 線，或切到其他
   frontend 子問題

## Non-Goals

這次重排不代表：

- 放棄 codegen refactor
- 立即追 full C++ operator parity
- 一次性重寫整個 declarator/parser 架構

原則是：

- 先清掉最常阻塞真實 C++ headers 的 frontend 關鍵路徑
- 再讓 bring-up case 往前推
- 最後才回到較長線的 codegen 結構整理
