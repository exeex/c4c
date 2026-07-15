# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 7
Current Step Title: Prove general function-signature lowering is structured-first

## Just Finished

Step 7 complete — audited general declaration/definition signature lowering:
present `signature_return_type_ref` and complete paired
`signature_params`/`signature_param_type_refs` already select structured facts
before the legacy `signature_text` parser. Added non-aggregate stale-text
coverage proving i32 return and parameter facts survive conflicting i64/double
declaration and definition text.

## Suggested Next

Select the next active-plan packet after reviewing the completed general
signature-lowering evidence and remaining structured type-reference consumers.

## Watchouts

Keep `signature_text` only as output or an explicit absent/incomplete-metadata
compatibility path. Do not absorb signature rendering changes,
function-reference reachability scans, globals, Raw-BIR, target lowering, or
unrelated call semantics.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_lir_function_signature_type_ref|backend_lir_to_bir_interface)$'`
(2/2). No canonical log files changed.
