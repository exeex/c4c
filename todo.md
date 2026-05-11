Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire LIR and HIR-to-LIR Final-Output Bridges

# Current Packet

## Just Finished

Step 5 - Retire LIR and HIR-to-LIR Final-Output Bridges:
Fenced LIR `type_decls` as a legacy output/verifier compatibility shadow in
comments and test-visible wording. `struct_decls` are now documented as the
printer/backend authority when populated, and verifier parsing of
`type_decls` is documented as a shadow-parity check only.

Changed files:

| File | Result |
| --- | --- |
| `src/codegen/lir/ir.hpp` | Reworded `type_decls` as the legacy compatibility shadow and `struct_decls` as the structured printer/backend path. |
| `src/codegen/lir/verify.cpp` | Documented `verify_struct_decl_shadows` as parsing legacy declarations only for shadow parity. |
| `src/codegen/lir/lir_printer.cpp` | Clarified that printed type declarations come from structured `struct_decls` when present. |
| `tests/frontend/frontend_lir_extern_decl_type_ref_test.cpp` | Updated the focused assertion message to name `struct_decls` as the emitted declaration source. |
| `todo.md` | Recorded this Step 5 packet result and proof. |

## Suggested Next

Continue Step 5 by inspecting the remaining HIR-to-LIR final-output bridge
comments or compatibility names that could still imply rendered LLVM text is
semantic authority when structured IDs or typed mirrors are present.

## Watchouts

- This packet intentionally preserved output spelling, verifier diagnostics,
  and legacy `type_decls` storage.
- No behavior changed; this was a boundary/documentation slice for the retained
  compatibility shadow.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Proof command:
`cmake --build build --target frontend_lir_extern_decl_type_ref_test backend_prepare_structured_context_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_extern_decl_type_ref|backend_prepare_structured_context)$'`

Result: passed. Output was written to `test_after.log`.

Also ran `git diff --check`: passed.
