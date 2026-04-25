Status: Active
Source Idea Path: ideas/open/113_backend_struct_decl_layout_table_dual_path.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Broader Parity And Regression Checkpoint

# Current Packet

## Just Finished

Step 6 - Broader Parity And Regression Checkpoint ran the broader validation
checkpoint after the Step 3-5 structured-first backend consumer conversions.
The full default build, full `ctest`, and backend target rebuild all passed.

Changed files:

- `todo.md`
- `test_after.log`

Structured-first coverage confirmed for the Step 3-5 converted consumers:

- Step 3 converted the direct member layout query helpers to prefer the backend
  structured layout table before legacy layout facts.
- Step 4 converted value/member lowering paths so structured layout facts flow
  through aggregate store/load and member-address consumers.
- Step 5 converted global GEP address and dynamic global array addressing
  helpers to prefer structured layout when called from member lowering paths,
  while preserving the legacy no-table overloads.

Remaining intentional fallback-only backend paths: legacy public overloads for
`resolve_global_gep_address()`, `resolve_relative_global_gep_address()`,
`resolve_global_dynamic_pointer_array_access()`, and
`resolve_global_dynamic_aggregate_array_access()` still route through null-table
wrappers for callers without module structured layout state. Core parity and
legacy layout functions also remain fallback-only by design where they serve as
the compatibility path under structured lookup miss or absent table state.

ABI classification files, global initializer files, globals/module files,
unrelated MIR files, test expectations, `plan.md`, and the source idea were not
changed.

## Suggested Next

Next coherent packet: supervisor review/closeout for the Step 3-6 structured
layout checkpoint, or delegate the next plan step if more structured layout
coverage remains in the active runbook.

## Watchouts

The broad checkpoint is green, but remaining fallback-only paths are intentional
compatibility paths rather than converted consumers. Future conversion work
should distinguish public legacy overloads/null-table wrappers from consumer
sites that can actually receive `structured_layouts_`.

## Proof

Delegated Step 6 proof passed and wrote `test_after.log`:

`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure && cmake --build build-backend --target c4c_backend -j) > test_after.log 2>&1`

Result: default build completed, full ctest passed 2980/2980, and the backend
`c4c_backend` target rebuilt successfully.
