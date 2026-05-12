Status: Active
Source Idea Path: ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add focused global/type/layout tests

# Current Packet

## Just Finished

Completed Step 3 for the selected generated metadata-rich aggregate-global
path by reviewing focused global/type/layout coverage.

Coverage decision:
- `check_global_initializer_lowering_prefers_structured_layout_table` covers
  structured success for aggregate globals carrying `LirGlobal::llvm_type_ref`,
  including a stale final `llvm_type` string that must not become authority.
- The same focused test covers stale structured metadata rejection and missing
  `StructNameId` rejection on the selected generated path.
- Its legacy fallback fixture covers retained raw/no-id compatibility through
  the intentional string-keyed `TypeDeclMap` route.
- Lower-level layout and initializer helpers in this file also cover structured
  success, stale/missing structured metadata rejection, and fallback status, so
  no additional Step 3 test was genuinely missing.

## Suggested Next

Execute Step 4 of `plan.md`: validate the selected aggregate-global fence and
prepare handoff for supervisor lifecycle close/switch review.

## Watchouts

- Idea 185 is a boundary audit plus selected generated-path repair; do not
  rewrite all legacy type declaration parsing in one pass.
- Preserve printer/final spelling and raw imported LIR compatibility unless a
  selected generated metadata-rich path proves it must fail closed.
- Pointer initializers carrying `initializer_function_link_name_ids` remain a
  separate candidate if the supervisor wants a second selected path, but Step 3
  coverage is complete for the currently selected aggregate-global
  `llvm_type_ref` path.
- `BackendStructuredLayoutTable` is still keyed by final type spelling for the
  structured-to-legacy bridge, but `lookup_backend_aggregate_type_ref_layout_result`
  selects entries by `StructNameId`; prefer extending that route over adding new
  string-keyed generated-path authority.
- Backend freeze closure remains owned by idea 188.

## Proof

Validation command:
`git diff --check -- tests/backend/backend_prepare_structured_context_test.cpp todo.md`

Result: passed.

Supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed. `test_after.log` reports 100% tests passed, 0 failed out of
109 backend tests.
