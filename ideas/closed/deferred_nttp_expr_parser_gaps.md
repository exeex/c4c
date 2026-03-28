# Deferred NTTP Expression Parser Gaps

## Status

Complete.

## Summary

The HIR-side deferred NTTP text evaluator still fails on several expression
forms beyond the already covered direct bound-name, parenthesized, unary-bool,
logical/equality, and literal cases.

## Repro Cases

1. Arithmetic default:

```cpp
template<int N, int M = N + 2>
struct Buffer {
    int data[M];
};

Buffer<3> global_buffer;
```

Observed HIR today:

- `global global_buffer: struct Buffer_N_3_M_0`

Expected:

- `struct Buffer_N_3_M_5`
- `field data: int[5] ... size=20 align=4`

2. Template static-member lookup default:

```cpp
template<int N>
struct Count {
    static constexpr int value = N;
};

template<int N, int M = Count<N>::value + 2>
struct Buffer {
    int data[M];
};
```

Observed HIR today:

- `global global_buffer: struct Buffer_N_3_M_0`

3. Pack-size default:

```cpp
template<typename... Ts, int M = sizeof...(Ts)>
struct Buffer {
    int data[M];
};

Buffer<int, long, char> global_buffer;
```

Observed HIR today:

- `global global_buffer: struct Buffer`

Expected:

- `struct Buffer_Ts_int_long_char_M_3` or equivalent concrete instantiation
- `field data: int[3] ... size=12 align=4`

## Notes

- Parser-side deferred NTTP evaluation already has a richer token-based path in
  `src/frontend/parser/types.cpp`.
- HIR lowering currently uses a lightweight text parser in
  `src/frontend/hir/ast_to_hir.cpp`.
- The direct binding case `M = N` works and is covered by
  `cpp_hir_template_deferred_nttp_expr`.

## Suggested Follow-up

- Align the HIR deferred-expression evaluator with the parser-side reference
  behavior for arithmetic, template static-member lookup, and `sizeof...(pack)`
  before expanding Step 4 beyond helper isolation.

## Completion Notes

- Added focused internal HIR regressions for the three documented missing
  mechanisms:
  `cpp_hir_template_deferred_nttp_arith_expr`,
  `cpp_hir_template_deferred_nttp_static_member_expr`, and
  `cpp_hir_template_deferred_nttp_sizeof_pack_expr`.
- The deferred NTTP path now materializes arithmetic defaults, dependent
  template static-member lookups, and `sizeof...(pack)` defaults into concrete
  HIR instantiations for the repro cases tracked by this idea.
- Step 4 revalidation kept the deferred-NTTP HIR subset green and preserved the
  existing full-suite status with no new failures introduced by this work.

## Leftover Issues

- No remaining deferred NTTP gaps were found inside the documented arithmetic,
  static-member, and pack-size scope of this idea.
- The `sizeof...(Ts)` repro remains an internal HIR regression rather than a
  Clang-conformance target because the specific template-parameter ordering in
  the source example is non-standard and rejected by Clang.
- The repository still has seven pre-existing unrelated full-suite failures in
  the existing template/sema and initializer-list cluster; this idea did not
  expand to address them.
