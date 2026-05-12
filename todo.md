Status: Active
Source Idea Path: ideas/open/178_global_aggregate_layout_structured_boundary.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Thread Structured Layout Identity To The Global Consumer

# Current Packet

## Just Finished

Completed `plan.md` Step 2 by threading metadata-rich global aggregate type
identity into the BIR global layout and initializer consumer.

Concrete work completed:

- `BackendStructuredLayoutEntry` now retains the producing `StructNameId`, and
  structured layout lookup has a dedicated `LirTypeRef` route that resolves
  metadata-rich aggregate globals by id instead of by rendered
  `global.llvm_type` text.
- `lower_minimal_global_impl` uses `LirGlobal::llvm_type_ref` when present for
  aggregate global size/alignment and root initializer lowering; the existing
  text-only overloads remain the explicit compatibility fallback for no-metadata
  globals.
- Aggregate initializer lowering now has a structured-root entry point,
  `lower_aggregate_initializer_for_type_ref`, while recursive field and
  no-metadata paths retain the existing text-based compatibility behavior.
- Focused backend coverage now uses a stale rendered global type spelling with
  a valid `StructNameId` mirror, proving the selected metadata-rich route does
  not choose layout authority solely through `global.llvm_type`.

## Suggested Next

Proceed to the next plan step for this route: exercise the structured global
layout identity through the downstream backend consumer that prepares or emits
the lowered BIR global storage, using the stale-rendered-text case as the guard.

## Watchouts

- The structured-root initializer entry point intentionally fails closed if a
  present `StructNameId` has no structured layout table entry; it does not fall
  back to `global.llvm_type`.
- Keep legacy no-metadata compatibility explicit and isolated; the text-only
  overloads are still used for no-metadata globals.
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
