# Current Packet

Status: Active
Source Idea Path: ideas/open/762_lir_module_declaration_type_shadow_convergence.md
Source Plan Path: plan.md
Current Step ID: 6
Current Step Title: Prove global type consumers use the structured global type ref

## Just Finished

Step 6 complete — `lower_minimal_global` now derives scalar, array, aggregate,
and initializer type handling from `llvm_type_ref::render_llvm()` whenever
metadata is present; `llvm_type` remains the explicit absent-metadata
compatibility path. Nearby global coverage rejects a stale rendered scalar
shadow against complete structured metadata transactionally.

## Suggested Next

Select the next active-plan packet after reviewing Step 6's global-consumer
result and any remaining structured type-reference consumers.

## Watchouts

Keep `llvm_type` only as an explicit legacy/no-metadata compatibility or
emission path. Do not absorb signature rendering, function-reference
reachability scans, Raw-BIR, target lowering, or unrelated initializer
semantics.

## Proof

Passed: `cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_lir_global_type_ref|backend_lir_to_bir_interface)$'`
(2/2). Proof log: `test_after.log`.
