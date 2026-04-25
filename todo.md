Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Add Structured-First Layout Access Helpers

# Current Packet

## Just Finished

Step 2 helper target completed: added
`stmt_emitter_detail::structured_layout_align_bytes` as a reusable
structured-first alignment accessor. It derives aggregate alignment from
`LirStructDecl` field type refs only when structured coverage exists, legacy
field parity was checked and matched, the aggregate is not a union, and the
derived alignment matches the legacy alignment. Otherwise it returns
`std::nullopt` so callers keep legacy/default fallback behavior.

Converted active `legacy_decl` authority sites:
- `src/codegen/lir/hir_to_lir/core.cpp` `stmt-object-align`
  `object_align_bytes`: now prefers the structured helper and falls back to
  `layout.legacy_decl->align_bytes` only when the helper declines coverage.
- `src/codegen/lir/hir_to_lir/hir_to_lir.cpp` `module-object-align`
  `object_align_bytes`: same structured-first helper plus legacy fallback.

## Suggested Next

Next coherent packet should decide whether to extend the same guarded helper
pattern to aggregate payload sizing or move to one of the Step 3 field lookup
paths that already has structured identity available.

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage; do not remove
  `StructuredLayoutLookup::legacy_decl`.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- `structured_layout_align_bytes` intentionally rejects unions because the
  current structured union mirror is `[N x i8]` and does not encode union
  member alignment.
- Alignment derivation remains conservative for unknown raw LLVM type text,
  missing nested `LirStructDecl`s, parity absence, parity mismatch, or derived
  alignment mismatch; those cases continue to use legacy fallback.
- Deferred active size/align authority: variadic aggregate payload sizing and
  `va_arg` aggregate payload sizing still need structured size metadata or a
  similarly guarded structured-size derivation before demotion.

## Proof

Passed delegated proof, with output preserved in `test_after.log`:
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|cpp_hir_record_field_array_layout|cpp_hir_record_packed_aligned_layout|cpp_positive_sema_inherited_base_aggregate_init_runtime_cpp|abi_)') > test_after.log 2>&1`
