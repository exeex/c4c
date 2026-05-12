Status: Active
Source Idea Path: ideas/open/178_global_aggregate_layout_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Fail Closed On Stale Or Mismatched Global Metadata

# Current Packet

## Just Finished

Completed `plan.md` Step 3 by making metadata-rich global aggregate layout and
initializer lowering fail closed when structured identity is stale, missing,
opaque, or mismatched.

Concrete work completed:

- `lookup_backend_aggregate_type_ref_layout_result` no longer falls back to
  rendered aggregate text when a `LirTypeRef` lacks a live `StructNameId`, has
  stale mirror text, has no structured table entry, or hits a structured/legacy
  parity mismatch.
- `lower_aggregate_initializer_for_type_ref` now uses the same fail-closed
  identity checks before choosing a root structured layout name.
- The legacy text-only aggregate layout and initializer APIs remain the explicit
  no-metadata compatibility fallback.
- Focused backend coverage now includes stale, missing, opaque, and parity
  mismatched metadata-rich type-ref cases, plus a stale global metadata case
  whose rendered text has a legacy type declaration that text authority would
  have accepted.

## Suggested Next

Proceed to the next supervisor-selected packet for the active global aggregate
layout route, likely a review or downstream consumer check that confirms the
new fail-closed behavior is the intended route boundary.

## Watchouts

- Text-only lookup by rendered `%struct.Name` still preserves the route-local
  legacy fallback and can report parity mismatch while returning a layout; the
  stricter fail-closed rule is limited to metadata-rich `LirTypeRef` paths.
- `global.llvm_type` may be stale for a metadata-rich global as long as
  `llvm_type_ref` carries the live structured spelling and id; stale
  `llvm_type_ref` text now fails.
- Do not broaden into byval copy, AArch64 direct-LIR bridge retirement,
  function-pointer signature identity, or the closure gate.
- Do not weaken tests or mark supported global aggregate cases unsupported as
  proof of progress.
- `parse_global_address_initializer` remains text-based for raw initializer
  payloads; this packet only changes the root global aggregate layout authority.
- Nearby same-feature guards:
  `tests/backend/backend_prepare_structured_context_test.cpp`
  `check_aggregate_initializer_prefers_structured_layout_table` and
  `check_global_initializer_lowering_prefers_structured_layout_table`;
  `tests/frontend/frontend_lir_global_type_ref_test.cpp` global
  `StructNameId` and same-text/different-id equality checks;
  global pointer initializer handling through
  `parse_global_gep_initializer` and `resolve_pointer_initializer_offsets`;
  same-module aggregate global load consumers in
  `tests/backend/backend_x86_handoff_boundary_multi_defined_call_test.cpp`.

## Proof

Ran the delegated proof exactly:

`set -o pipefail; { cmake --build build --target backend_prepare_structured_context_test frontend_lir_global_type_ref_test && ctest --test-dir build -R 'backend_prepare_structured_context|frontend_lir_global_type_ref' --output-on-failure; } 2>&1 | tee test_after.log`

Result: passed. `test_after.log` contains the successful focused build and
CTest output for `frontend_lir_global_type_ref` and
`backend_prepare_structured_context`.
