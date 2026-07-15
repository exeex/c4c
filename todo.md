# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement and prove the selected struct-declaration authority surface

## Just Finished

Step 2 complete — `backend_lir_to_bir_interface` now derives both retained
`type_decls` lines from the structured `LirStructDecl` carrier, verifies the
positive packed/recursive declaration pair, and proves Raw-BIR receives its
structured names, fields, and references.  The same fixture mutates only one
legacy shadow (`%struct.Outer = type { i64 }`) and requires
`lir::verify_module` to reject it before BIR import.  Production authority
remains `LirModule::struct_decls`; `type_decls` is only a checked
compatibility/emission shadow.

## Suggested Next

Step 3: reassess the three excluded module surfaces against the completed
struct row and decide whether 762 can close or needs one further bounded row;
do not implement extern, function-signature, or global text routes as part of
this completed packet.

## Watchouts

`type_decls` must never be reparsed to recover selected struct identity or
layout when the matching `LirStructDecl` exists; stale text fails closed in the
verified selected seam.  Keep extern `return_type_str`, function
`signature_text`, and global `llvm_type` out of this row.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build
--output-on-failure -R '^backend_lir_to_bir_interface$'` (1/1). Build emitted
only existing deprecated runtime-text constructor warnings. Test output:
`test_after.log`.
