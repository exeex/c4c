Status: Active
Source Idea Path: ideas/open/115_hir_to_lir_layout_legacy_decl_demotion.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Demote Const Initializer And Field Lookup Paths

# Current Packet

## Just Finished

Step 3 accounting/proof completed after the Step 2 helper work:
const-initializer aggregate lookup no longer selects
`StructuredLayoutLookup::legacy_decl` as declaration authority. It retains
`lookup_structured_layout(..., "const-init-aggregate")` for structured/legacy
observation and parity reporting, then uses the existing `mod_.struct_defs`
lookup path for declaration identity.

Focused HIR-to-LIR `legacy_decl` scan found no remaining active semantic
authority in the Step 3 paths. Nested field lookup and aggregate GEP/lvalue
field-chain paths already use structured identity where available; current
`legacy_decl` reads are limited to helper parity gates, observation reporting,
and structured-first fallback behavior.

## Suggested Next

Next coherent packet should move to Step 5 and consolidate any remaining
selected HIR-to-LIR size/alignment consumers, then classify the final
`legacy_decl` scan results in `todo.md`.

## Watchouts

- Keep `legacy_decl` fallback for incomplete structured coverage; do not remove
  `StructuredLayoutLookup::legacy_decl`.
- Do not edit `src/backend/mir/` as part of this route.
- Do not weaken tests or add testcase-shaped shortcuts.
- Step 3 was an accounting/proof packet only; no code churn was needed because
  the reviewed implementation had already demoted const-init authority and the
  field-chain/GEP paths did not expose active `legacy_decl` authority.
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
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_|positive_sema_ok_call_variadic_aggregate_runtime_c|cpp_positive_sema_inherited_base_aggregate_init_runtime_cpp|cpp_hir_record_field_array_layout|cpp_hir_record_packed_aligned_layout|abi_)') > test_after.log 2>&1`

Result: build reported no work to do; CTest passed 11/11 tests.
