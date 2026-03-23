# Frontend Priority Plan

## Goal

先完成 template lazy instantiation / compile-time type realization 的基礎重構，
再用 EASTL `type_traits.h` bring-up 當第一個主要驗證入口，
之後才往上推 `integer_sequence.h`、`tuple_fwd_decls.h`、`utility.h`、
`tuple.h`、`memory.h`。

目前主線不是 `vector.h`，也不是更高層容器功能。
優先順序改成先把最底層 template instantiation / type-trait 基礎打穩。

## Priority Order

1. Lazy template type instantiation infrastructure
   - follow [ideas/template_lazy_instantiation_plan.md](/workspaces/c4c/ideas/template_lazy_instantiation_plan.md)
   - move dependent template type/default/member-type realization toward compile-time-engine fixpoint
2. EASTL `type_traits.h` bring-up
   - use `type_traits` as the first real validation surface for the new lazy path
3. EASTL `internal/integer_sequence.h`
4. EASTL `internal/tuple_fwd_decls.h`
5. EASTL `utility.h`
6. EASTL `tuple.h`
7. EASTL `memory.h`
8. 之後才回到 `vector.h` 與更高層 EASTL header

## Main Track

主線重構入口：

- `ideas/template_lazy_instantiation_plan.md`

主測試入口：

- `tests/cpp/eastl/eastl_type_traits_simple.cpp`
- target: `eastl_type_traits_simple_workflow`

拆分中的 header probes：

- `tests/cpp/eastl/eastl_integer_sequence_simple.cpp`
- `tests/cpp/eastl/eastl_tuple_fwd_decls_simple.cpp`
- `tests/cpp/eastl/eastl_utility_simple.cpp`
- `tests/cpp/eastl/eastl_tuple_simple.cpp`
- `tests/cpp/eastl/eastl_memory_simple.cpp`

## Why This Order

最近把 EASTL case 拆開後可以更清楚看見：

- `vector.h` 會把很多更深層 header 一起拉進來
- 這會讓最早 blocker 不夠清楚
- `type_traits.h` 才是目前更合理的第一個穩定入口

但目前更前面的真 blocker 其實不是單一 EASTL header，
而是 dependent template type realization 的責任分散在 parser、
`ast_to_hir`、compile-time engine 三邊。

目前新的主測試已經能直接暴露這個缺口：

- `object has incomplete type: is_signed_helper`

在這個問題清掉之前，往上追 `utility.h` / `tuple.h` / `memory.h`
的成本都偏高，因為很多錯誤只是連鎖反應。

所以現在的主順序應該是：

1. 先把 lazy template type instantiation 主線做對
2. 再把 `type_traits.h` 當成第一個大型驗證面
3. 之後才往上推其他 EASTL header

## Immediate Success Condition

以下成立時，可以往下一層 header 推進：

- `ideas/template_lazy_instantiation_plan.md` 的 Phase 1-5 至少完成一條可工作的窄路徑
- `eastl_type_traits_simple_workflow` host compile / host run / c4cll frontend / backend / runtime 全綠
- `type_traits.h` 內常用 traits 與 alias templates 至少有一組穩定 smoke coverage
- 不再先撞上 `is_signed_helper` 這類基礎 trait 展開問題

## Concrete Next Actions

1. 先實作 lazy template type instantiation 主線
   - 以 [ideas/template_lazy_instantiation_plan.md](/workspaces/c4c/ideas/template_lazy_instantiation_plan.md) 為主
   - parser 只保留 dependent NTTP expression / member-type / alias-template 結構
   - `ast_to_hir` 只搬運 pending type work，不再承擔越來越多 eager template semantics
   - compile-time engine 擴成 type-driven fixpoint：
     從實際需要具象化的 template type / alias / member typedef / base 出發做 lazy instantiate
   - dependent NTTP default 求值移到 bindings concrete 之後
   - concrete NTTP value 求出後，先做 specialization selection，再做 base/static member/operator() propagation
2. 用 `type_traits` 當第一個驗證面，修掉 `eastl_type_traits_simple.cpp` 暴露的第一個 blocker
   - `is_signed_helper` incomplete type
   - inherited `::value`
   - inherited `operator()`
3. 保持 `type_traits` 測試小而清楚，優先確保錯誤定位乾淨
   - 先用最小 probe 把 lazy 路徑隔離：
     `template_alias_deferred_nttp_expr_runtime.cpp`
   - 再用較大的 probe 驗證整條 EASTL trait 鏈：
     `inherited_static_member_lookup_runtime.cpp`
     `eastl_type_traits_signed_helper_base_expr_parse.cpp`
4. 視修正結果補更多 `type_traits.h` smoke cases：
   - signed / unsigned traits
   - `is_same`
   - `is_const`
   - `is_reference`
   - `remove_cv`
   - `remove_reference`
   - `add_lvalue_reference`
   - `conditional_t`
   - `enable_if_t`
5. `type_traits` 穩定後，再往 `eastl_integer_sequence_simple.cpp` 推
6. 之後依序往 `tuple_fwd_decls`、`utility`、`tuple`、`memory` 推進

## Rule

- 先完成 lazy template type instantiation 的主幹，不在 parser 上繼續累積 dependent-evaluation 特例
- 先解最底層 header 的第一個真 blocker
- 不為了追高層 header 暫時把底層錯誤掩蓋掉
- 保留 split tests，讓 fail 直接暴露
- 每往上一層 header，都要確認下層入口已經夠穩定

## Implementation Direction

- 主線優先不是單補 `type_traits` parser 特例，而是先完成 lazy template type instantiation 基礎設施
- `type_traits` 是第一個主要驗證面，不是獨立於這條主線之外的特案
- parser 的責任：
  保留足夠的 deferred template arg / member-type / alias-template 資訊，
  尤其是 dependent NTTP expression 與 alias-template 參數映射
- compile-time engine 的責任：
  在 bindings concrete 後解析 deferred arg refs，產生 concrete NTTP values，
  並驅動 specialization selection 與 template type lazy instantiation fixpoint
- HIR lowering 的責任：
  消費 compile-time 決定後的 concrete struct instantiations，
  做 base_tags / method / static member propagation
- 這樣的分層可以避免：
  parser 把未知 expression 偷偷降成 `0`、
  `ast_to_hir` 直接承擔越來越多 template expression semantics、
  以及 alias template / specialization / inherited lookup 各自維護不同版本的 instantiation 規則
