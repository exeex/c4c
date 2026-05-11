Status: Active
Source Idea Path: ideas/open/168_compatibility_bridge_retirement.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Retire LIR and HIR-to-LIR Final-Output Bridges

# Current Packet

## Just Finished

Step 5 - Retire LIR and HIR-to-LIR Final-Output Bridges:
Fenced LIR `signature_text` as retained final LLVM/output spelling plus a
legacy no-metadata compatibility fallback. Structured signature mirrors
(`signature_return_type_ref`, `signature_params`, and
`signature_param_type_refs`) are now explicitly documented as the
verifier/backend authority when present.

Changed files:

| File | Result |
| --- | --- |
| `src/codegen/lir/ir.hpp` | Documented `signature_text` as final output spelling and legacy no-metadata payload, with structured signature fields preferred by verifier/backend paths. |
| `src/codegen/lir/verify.cpp` | Clarified that signature-text parsing only checks retained output spelling; structured signature mirrors are the authority despite historical helper naming. |
| `src/codegen/lir/lir_printer.cpp` | Clarified that `signature_text` remains final LLVM/output header spelling while `LinkNameId` is semantic identity. |
| `src/backend/bir/lir_to_bir/call_abi.cpp` | Documented return/parameter signature-text parsers as legacy hand-built/no-metadata fallbacks behind structured metadata. |
| `tests/frontend/frontend_lir_function_signature_type_ref_test.cpp` | Updated printer assertion messages to describe `signature_text` as final output spelling. |
| `tests/backend/backend_prepare_structured_context_test.cpp` | Updated the structured-drift failure message to name final-output `signature_text` versus structured return metadata. |
| `todo.md` | Recorded this Step 5 packet result and proof. |

## Suggested Next

Continue Step 5 by inspecting remaining HIR-to-LIR final-output bridge comments
around unresolved reference scans and legacy reference payloads, then decide
whether they need the same explicit "fallback only" wording or are ready for
plan-owner review.

## Watchouts

- This packet intentionally preserved output spelling, verifier diagnostics,
  ABI lowering behavior, and retained `signature_text` storage.
- No behavior changed; this was a boundary/documentation slice for the retained
  final-output/legacy fallback boundary.
- Existing focused tests already prove drifted `signature_text` does not
  override structured verifier/backend metadata; no new fallback gap was found
  in the owned files.
- The pre-existing untracked `review/168_step4_hir_bridge_route_review.md`
  remains transient and was not modified.
- No current blockers.

## Proof

Proof command:
`cmake --build build --target frontend_lir_function_signature_type_ref_test backend_prepare_structured_context_test && ctest --test-dir build -j --output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_prepare_structured_context)$'`

Result: passed. Output was written to `test_after.log`.

Also ran `git diff --check`: passed.
