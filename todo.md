# Current Packet

Status: Active
Source Idea Path: ideas/open/809_lir_phi_scalar_dereference_load_authority.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Repair and focus-proof the scalar dereference-load handoff

## Just Finished

809 Step 1 complete: scalar `UnaryOp::Deref` now gives its `LirLoadOp` result a native current-function `LirValueId`, and nearby coverage proves the ternary PHI receives that exact load-result authority while unknown authority is rejected.

## Suggested Next

Supervisor acceptance of Step 1, then execute Step 2's supervisor-selected fresh full-baseline gate and parent-return decision.

## Watchouts

Focused proof passed only; do not treat it as clearance for 806/804 without the Step 2 supervisor-accepted full baseline. Do not reopen postfix, `fneg`, `xor`, scalar unary-minus, CFG, PHI verification, or generic conditional lowering.

## Proof

Fresh direct coverage: `cmake --build --preset default && ./build/tests/frontend/frontend_lir_call_type_ref_test` (passed). Fresh required proof: `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^(frontend_hir_tests|llvm_gcc_c_torture_src_20060910_1_c)$' > test_after.log 2>&1` (passed, 2/2); log: `test_after.log`. Full baseline requires later supervisor acceptance.
