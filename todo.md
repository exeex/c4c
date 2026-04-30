# Current Packet

Status: Active
Source Idea Path: ideas/open/138_lir_bir_backend_aggregate_layout_type_decl_text_bridge_cleanup.md
Source Plan Path: plan.md
Current Step ID: Step 2
Current Step Title: Make Backend Layout Lookup Structured-Primary

## Just Finished

Step 2 backend aggregate layout lookup scan completed for idea 138. The direct
`lookup_backend_aggregate_type_layout()` consumers in `aggregate.cpp`,
`memory/addressing.cpp`, and the local/global wrapper helpers all route through
the structured-aware overload when a `BackendStructuredLayoutTable` is
available; the remaining direct `compute_aggregate_type_layout()` paths are the
legacy no-structured overloads or the internal structured/text parity and
fallback implementation. No remaining Step 2 implementation packet was found.

## Suggested Next

Next coherent packet: advance to Step 3 by making aggregate initializer layout
coverage prove structured-present behavior and structured-missing fallback
through `lower_aggregate_initializer()` in `global_initializers.cpp`.

Suggested proof for that packet:
`cmake --build build --target backend_prepare_structured_context_test backend_lir_to_bir_notes_test > test_after.log 2>&1 && ctest --test-dir build -R '^(backend_prepare_structured_context|backend_lir_to_bir_notes)$' --output-on-failure >> test_after.log 2>&1`.

## Watchouts

Step 3 and Step 4 are still expected to cover aggregate and global initializer
consumers, but the Step 2 lookup primitive and addressing/local/global wrapper
routes already prefer structured layout data whenever the caller supplies the
structured table. Do not delete `LirModule::type_decls`; the fallback route is
still needed when `struct_decls` is absent.

## Proof

Proof passed for this todo-only scan packet:
`git diff --check -- todo.md`.
