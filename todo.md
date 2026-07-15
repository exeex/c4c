# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Make structured extern return type authoritative

## Just Finished

Step 3 complete — the AST-backed route audit found an in-scope extern-return
authority defect: `LirExternDecl::return_type` is structured and verifier-
checked against `return_type_str`, yet
`BirFunctionLowerer::lower_extern_decl` lowers the rendered shadow first and
uses the structured ref only as fallback. This does not require a missing-
carrier blocker. A fresh build and the exact paired baseline passed 2/2; the
before-run is in `test_before.log`.

## Suggested Next

Step 4: lower extern return data from `decl.return_type` first in
`BirFunctionLowerer::lower_extern_decl`; consult `decl.return_type_str` only
when the structured carrier is absent. Add stale-shadow proof spanning
`frontend_lir_extern_decl_type_ref` and `backend_lir_to_bir_interface`, then
build and run exactly those two tests while preserving valid LLVM output.

## Watchouts

A present `decl.return_type` must always win over `return_type_str`; rendered
text is compatibility fallback only for an absent structured carrier. Do not
broaden this packet into function `signature_text`, global `llvm_type`, printer
rewrites, Raw-BIR, target lowering, or unrelated module surfaces. Do not weaken
verifier behavior merely to admit a stale shadow.

## Proof

Baseline passed before Step 4: fresh build, then `ctest --test-dir build -j
--output-on-failure -R '^(frontend_lir_extern_decl_type_ref|backend_lir_to_bir_interface)$'`
(2/2); output is recorded in `test_before.log`. Required packet proof: fresh
build and the same exact paired command after the change, recorded for the
supervisor's matching regression comparison. Preserve valid LLVM output.
