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
2. 保持 `type_traits` 測試小而清楚，優先確保錯誤定位乾淨
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
