# Current Packet

Status: Active
Source Idea Path: ideas/open/801_lir_anonymous_aggregate_layout_type_facts.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Prove and publish the 754 handoff

## Just Finished

Step 2 is supervisor-accepted. The direct-complex by-value call shares one
native anonymous `{ float, float }` layout across the callee signature,
`arg_type_refs`, and `structured_args`. The verifier recursively validates
anonymous layouts and rejects incoherent named structured `LirCallOp` carriers.
The fresh focused proof passed 3/3; the comparable full gate added no failures.

## Suggested Next

Step 3: prove the positive and malformed native field-layout contract, then
publish the precise handoff for 754 Step 3. 754 is the receiver only; do not
perform its extractvalue-row work or claim its handoff accepted.

## Watchouts

Do not parse `LirTypeRef` display text, use rendered diagnostics as argument
type authority, weaken `LirCallOp` mirror/signature checks, or add
extractvalue field/index/result validation, Raw-BIR, or generic aggregate
work. Keep the 754 handoff limited to native layout facts that its unchanged
Step 3 may consume.

## Proof

Accepted Step 2 proof: `cmake --build --preset default && ctest --test-dir
build -j --output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|
frontend_lir_function_signature_type_ref|backend_)$'`: 3/3 passed
(`frontend_hir_tests`, `frontend_lir_call_type_ref`,
`frontend_lir_function_signature_type_ref`). The fresh comparable full command
completed 3035/3037; its only failures were pre-existing
`cpp_qualified_template_call_template_arg_perf` and the 806-owned
`llvm_gcc_c_torture_src_20060910_1_c`, so the failure set did not expand.
