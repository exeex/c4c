Status: Active
Source Idea Path: ideas/open/185_lir_to_bir_global_typedecl_compatibility_fence.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Select and fence a generated metadata-rich path

# Current Packet

## Just Finished

Completed Step 2 for the selected generated metadata-rich aggregate-global
path.

Selected path:
- Aggregate globals carrying `LirGlobal::llvm_type_ref`.

Implementation notes:
- `lower_minimal_global_impl` now routes metadata-rich aggregate globals through
  an explicit `lookup_structured_global_layout_result` fence.
- The fenced route accepts only lookups that actually used structured
  `StructNameId` layout metadata and rejects legacy fallback, stale structured
  text, or missing structured id metadata.
- Raw/no-id aggregate globals still use `global.llvm_type` through
  `lookup_global_layout_result`, preserving the existing textual `TypeDeclMap`
  compatibility path.
- `tests/backend/backend_prepare_structured_context_test.cpp` now covers the
  missing-`StructNameId` global case that would otherwise be able to lower via
  `%struct.Pair` final spelling.

## Suggested Next

Execute Step 3 of `plan.md`: add or review focused global/type/layout tests for
structured success, stale/missing structured metadata rejection, and retained
raw/no-id compatibility. The current packet already added one missing-id case,
so the next packet can either broaden focused coverage or decide whether Step 3
is satisfied by the existing structured/stale/fallback tests.

## Watchouts

- Idea 185 is a boundary audit plus selected generated-path repair; do not
  rewrite all legacy type declaration parsing in one pass.
- Preserve printer/final spelling and raw imported LIR compatibility unless a
  selected generated metadata-rich path proves it must fail closed.
- Pointer initializers carrying `initializer_function_link_name_ids` remain a
  separate candidate if the supervisor wants a second selected path, but this
  packet intentionally fenced only aggregate globals with `llvm_type_ref`.
- `BackendStructuredLayoutTable` is still keyed by final type spelling for the
  structured-to-legacy bridge, but `lookup_backend_aggregate_type_ref_layout_result`
  selects entries by `StructNameId`; prefer extending that route over adding new
  string-keyed generated-path authority.
- Backend freeze closure remains owned by idea 188.

## Proof

Validation command:
`git diff --check -- src/backend/bir/lir_to_bir/globals.cpp tests/backend/backend_prepare_structured_context_test.cpp todo.md`

Result: passed.

Supervisor-selected proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

Result: passed. `test_after.log` reports 100% tests passed, 0 failed out of
109 backend tests.
