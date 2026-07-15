# 810 Step 1 — GEP producer trace

## Reproduction

The delegated focused proof reproduces the failure without changing code:

```text
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^c_testsuite_src_00173_c$'
...
error: LirGepOp.result: authoritative GEP requires LirValueId result authority
```

`tests/c/external/c-testsuite/src/00173.c` contains two pointer postfix
increments: `b++` in the first character-printing loop and `dest++` in the
copy loop.  The first is reached first in source/lowering order.

## Exact authority and producer handoff

`verify_authoritative_gep` in `src/codegen/lir/verify.cpp:1271-1283` enters
the authoritative path if the result, base, or an index has authority, then
rejects a result without `LirValueId`.  The verifier dispatches every GEP
through this check at `verify_inst` (`src/codegen/lir/verify.cpp:1566-1573`).

For this testcase's `b++`, the immediate native path is:

```text
UnaryOp::PostInc
  -> emit_assignable_incdec_value (lvalue.cpp:619-654)
  -> emit_load_assignable_value (lvalue.cpp:562-576)
  -> LirGepOp construction (lvalue.cpp:643-649)
  -> verify_authoritative_gep
```

`emit_load_assignable_value` creates `loaded.value` with `fresh_value(ctx)`
and publishes it as a `LirLoadOp` result (lines 571-575).  `fresh_value` in
`src/codegen/lir/hir_to_lir/core.cpp:1438-1443` allocates a module
`LirValueId`, so the GEP base is a current-function authoritative SSA value.
The next construction in the pointer branch instead uses
`const std::string new_val = fresh_tmp(ctx)` and constructs
`LirGepOp{new_val, ..., loaded.value, false, {"i64 " + delta}}`.
`fresh_tmp` is display text only (core.cpp:1436), so the GEP result lacks a
native ID while its base makes it authoritative.  This is precisely the
`LirGepOp.result` rejection; it is not a verifier-contract defect.

The same function has the same construction for pointer `+=`/`-=` at
`lvalue.cpp:737-741`: it creates a `fresh_tmp` GEP result from `loaded.value`.
That is source-level evidence for one bounded producer family: native pointer
increment/decrement and pointer compound-add/sub result GEPs.  `00173.c`'s
`dest++` is a second direct instance of the postfix-increment member of that
family.

## Boundary

The existing partial `test_after.log` lists many other GEP-result failures,
but it does not identify their lowering producers and is diagnostic only.
No other testcase was selected or run for this trace-only packet.  Therefore
the evidence supports a bounded pointer-inc/dec (and adjacent pointer
compound-add/sub) producer packet only; it does **not** justify grouping the
remaining logged GEP failures.  Those require their own producer trace or a
separately scoped successor.

The focused CTest command does not write the canonical root `test_after.log`;
per packet ownership, the pre-existing partial root log was left untouched.
