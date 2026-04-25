Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Structured-First Layout Access Helpers

# Current Packet

## Just Finished

Step 2 / Step 4 structured payload-size target completed: added
`stmt_emitter_detail::structured_layout_size_bytes` as a reusable
structured-first aggregate size accessor. It derives size from `LirStructDecl`
field type refs only when structured coverage exists, legacy field parity was
checked and matched, and the derived size agrees with legacy
`HirStructDef::size_bytes`; otherwise it returns `std::nullopt` so callers keep
legacy fallback behavior.

Converted active aggregate payload-size sites:
- `src/codegen/lir/hir_to_lir/call/args.cpp` `variadic-aggregate-arg`: now
  prefers `structured_layout_size_bytes` and falls back to
  `layout.legacy_decl->size_bytes` only when the helper declines coverage.
- `src/codegen/lir/hir_to_lir/call/vaarg.cpp` `va_arg-aggregate`: now computes
  one aggregate payload size via the structured helper first and reuses it for
  zero-sized aggregate handling and AArch64 aggregate payload copies, falling
  back to the prior legacy union/non-union payload logic when structured size
  is unavailable.

## Suggested Next

Next coherent packet should move to Step 3 field lookup paths that already have
structured identity available, while preserving legacy fallback for incomplete
structured coverage.

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage; do not remove
  `StructuredLayoutLookup::legacy_decl`.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- `structured_layout_align_bytes` intentionally rejects unions because the
  current structured union mirror is `[N x i8]` and does not encode union
  member alignment.
- Size derivation remains conservative for unknown raw LLVM type text, missing
  nested `LirStructDecl`s, parity absence, parity mismatch, or derived-size
  mismatch; those cases continue to use legacy fallback.
- `structured_layout_size_bytes` allows zero as a valid structured result so
  zero-sized aggregate skip/zeroinitializer behavior remains explicit.

## Proof

Passed delegated proof, with output preserved in `test_after.log`:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|cpp_hir_record_field_array_layout|cpp_hir_record_packed_aligned_layout|cpp_positive_sema_inherited_base_aggregate_init_runtime_cpp|abi_)') > test_after.log 2>&1`
