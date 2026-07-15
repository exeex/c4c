# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove aggregate parameter lowering uses structured function signatures

## Just Finished

Step 5 complete — aggregate parameter collection now treats paired
`signature_params`/`signature_param_type_refs` as structured authority only
when complete; rendered `signature_text` is the legacy fallback for absent or
incomplete metadata. Backend coverage supplies a syntactically valid,
conflicting parameter shadow while preserving the complete structured ABI.

## Suggested Next

Supervisor to select the next coherent packet from the active runbook.

## Watchouts

This packet changed only aggregate parameter collection. `signature_text`
remains a legacy fallback only for absent or incomplete parameter metadata;
complete structured parameter pairs remain authoritative. Do not absorb
signature rendering, function-reference reachability scans, globals, Raw-BIR,
target lowering, or unrelated function consumers.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^backend_lir_to_bir_interface$'` (1/1). Per packet
authority, canonical regression logs were not modified.
