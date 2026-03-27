# Deferred NTTP Expression Parser Gaps

## Status

Open.

## Summary

The HIR-side deferred NTTP text evaluator still fails on expression forms beyond
direct bound-name reuse.

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

## Notes

- Parser-side deferred NTTP evaluation already has a richer token-based path in
  `src/frontend/parser/types.cpp`.
- HIR lowering currently uses a lightweight text parser in
  `src/frontend/hir/ast_to_hir.cpp`.
- The direct binding case `M = N` works and is covered by
  `cpp_hir_template_deferred_nttp_expr`.

## Suggested Follow-up

- Align the HIR deferred-expression evaluator with the parser-side reference
  behavior for arithmetic and template static-member lookup before expanding
  Step 4 beyond helper isolation.
