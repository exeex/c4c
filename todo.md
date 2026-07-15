# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make structured extern return type authoritative

## Just Finished

Step 4 complete — `BirFunctionLowerer::lower_extern_decl` now lowers a present
`LirExternDecl::return_type` first, passing its structured carrier into return
ABI lowering. It consults `return_type_str` only when that carrier is absent.
The frontend stale-mirror coverage and backend stale-return-shadow rejection
retain structured authority and verifier fail-closed behavior.

## Suggested Next

Supervisor to select the next coherent packet from the active runbook.

## Watchouts

The backend test confirms stale `return_type_str` remains rejected at module
verification; it intentionally does not weaken that boundary. Do not broaden
into function `signature_text`, global `llvm_type`, printer rewrites, Raw-BIR,
target lowering, or unrelated module surfaces.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_lir_extern_decl_type_ref|backend_lir_to_bir_interface)$'`
(2/2). Per packet authority, canonical regression logs were not modified.
