# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Prove aggregate parameter lowering uses structured function signatures

## Just Finished

Step 4 complete — accepted in `44ca942cf`. `BirFunctionLowerer::lower_extern_decl`
now lowers a present `LirExternDecl::return_type` first and consults
`return_type_str` only when the structured carrier is absent. The paired
frontend/backend focused proof passed 2/2 before and after, and the matching
regression guard was clean.

## Suggested Next

Step 5: audit and, only where needed, correct the aggregate-parameter ABI seam
in `src/backend/bir/lir_to_bir/aggregate.cpp` so complete
`signature_params`/`signature_param_type_refs` always win over
`signature_text`. Add a stale-signature-shadow backend LIR-to-BIR proof, then
build and run the exact affected backend interface test before and after.

## Watchouts

This packet owns only the aggregate parameter consumer seam. `signature_text`
may remain an explicit fallback for hand-built/no-metadata LIR, but never when
complete structured signature parameters and refs exist. Do not absorb
signature rendering, function-reference reachability scans, globals,
Raw-BIR, target lowering, or unrelated function consumers.

## Proof

Accepted Step 4 proof: `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R
'^(frontend_lir_extern_decl_type_ref|backend_lir_to_bir_interface)$'` (2/2)
before and after; matching regression guard clean. Step 5 requires a fresh
build plus the exact affected backend LIR-to-BIR interface test before and
after, with evidence retained for supervisor comparison. Per packet authority,
canonical regression logs remain supervisor-owned.
