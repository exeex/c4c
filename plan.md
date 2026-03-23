# Frontend Priority Plan

## Goal

先完成 EASTL `type_traits.h` bring-up，建立一個穩定的最底層 EASTL 入口，
再往上推 `integer_sequence.h`、`tuple_fwd_decls.h`、`utility.h`、
`tuple.h`、`memory.h`。

目前主線不是 `vector.h`，也不是更高層容器功能。
優先順序改成先把最底層 type-trait / template 基礎打穩。

## Priority Order

1. EASTL `type_traits.h` bring-up
2. EASTL `internal/integer_sequence.h`
3. EASTL `internal/tuple_fwd_decls.h`
4. EASTL `utility.h`
5. EASTL `tuple.h`
6. EASTL `memory.h`
7. 之後才回到 `vector.h` 與更高層 EASTL header

## Main Track

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

目前新的主測試已經能直接暴露第一個缺口：

- `object has incomplete type: is_signed_helper`

在這個問題清掉之前，往上追 `utility.h` / `tuple.h` / `memory.h`
的成本都偏高，因為很多錯誤只是連鎖反應。

## Immediate Success Condition

以下成立時，可以往下一層 header 推進：

- `eastl_type_traits_simple_workflow` host compile / host run / c4cll frontend / backend / runtime 全綠
- `type_traits.h` 內常用 traits 與 alias templates 至少有一組穩定 smoke coverage
- 不再先撞上 `is_signed_helper` 這類基礎 trait 展開問題

## Concrete Next Actions

1. 修掉 `eastl_type_traits_simple.cpp` 目前暴露的第一個 frontend blocker
   - `is_signed_helper` incomplete type
   - 後續 runtime / trait-dispatch 修正優先放在 lazy instantiation / compile-time state，
     不再把 NTTP expression resolution 繼續堆在 `ast_to_hir`
   - 這條線的預想實作細節：
     parser 只負責保留 dependent NTTP expression 的語法外形，
     例如把 `bool_constant<(T(-1) < T(0))>` 記成 deferred arg ref，
     而不是在 parser 階段試圖算出 `0/1`
   - 這條線的預想實作細節：
     alias template 不能只降成一般 `typedef_types_`，
     還要保留 alias template 的參數順序、NTTP/type 分類、以及被 alias 的 pending template-struct 形狀
   - 這條線的預想實作細節：
     `using bool_constant = integral_constant<bool, B>` 應該能在 alias application 時，
     把 `bool_constant<expr>` 重建成 pending `integral_constant<bool, expr>`
   - 這條線的預想實作細節：
     真正的 expression evaluation 應放在 lazy template struct instantiation，
     也就是 bindings 已 concrete 之後，而不是 AST parse 時
   - 這條線的預想實作細節：
     compile-time engine 應持有共用 helper，
     至少能處理：
     bool/int literal、forwarded NTTP names、簡單 cast、比較/邏輯運算、`Template<Args>::value`
   - 這條線的預想實作細節：
     concrete NTTP value 一旦求出，必須先走 specialization selection，
     再做 base propagation、static member lookup、operator() dispatch
2. 保持 `type_traits` 測試小而清楚，優先確保錯誤定位乾淨
   - 先用最小 probe 把 lazy 路徑隔離：
     `template_alias_deferred_nttp_expr_runtime.cpp`
   - 再用較大的 probe 驗證整條 EASTL trait 鏈：
     `inherited_static_member_lookup_runtime.cpp`
     `eastl_type_traits_signed_helper_base_expr_parse.cpp`
3. 視修正結果補更多 `type_traits.h` smoke cases：
   - signed / unsigned traits
   - `is_same`
   - `is_const`
   - `is_reference`
   - `remove_cv`
   - `remove_reference`
   - `add_lvalue_reference`
   - `conditional_t`
   - `enable_if_t`
4. `type_traits` 穩定後，再往 `eastl_integer_sequence_simple.cpp` 推
5. 之後依序往 `tuple_fwd_decls`、`utility`、`tuple`、`memory` 推進

## Rule

- 先解最底層 header 的第一個真 blocker
- 不為了追高層 header 暫時把底層錯誤掩蓋掉
- 保留 split tests，讓 fail 直接暴露
- 每往上一層 header，都要確認下層入口已經夠穩定

## Implementation Direction

- `type_traits` 這條主線的關鍵不是多補 parser 特例，而是把 dependent template work 往 lazy instantiation / compile-time state 收斂
- parser 的責任：
  保留足夠的 deferred template arg 資訊，尤其是 dependent NTTP expression 與 alias-template 參數映射
- compile-time engine 的責任：
  在 bindings concrete 後解析 deferred arg refs，產生 concrete NTTP values，並驅動 specialization selection
- HIR lowering 的責任：
  消費 compile-time 決定後的 concrete struct instantiations，做 base_tags / method / static member propagation
- 這樣的分層可以避免：
  parser 把未知 expression 偷偷降成 `0`、
  `ast_to_hir` 直接承擔越來越多 template expression semantics、
  以及 alias template / specialization / inherited lookup 各自維護不同版本的 instantiation 規則
