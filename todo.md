# Current Packet

Status: Active
Source Idea Path: ideas/open/754_lir_aggregate_vector_value_identity_convergence.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Verify row-specific index or mask facts

## Just Finished

Lifecycle switch: 801 is capability-complete. Its accepted Step 2--3 handoff
publishes the native anonymous `{ float, float }` layout shared structurally
by the direct-complex call callee signature, `arg_type_refs`, and
`structured_args`; the supplied fresh focused proof passed 3/3. No 754 Step 3
code or test work has been performed.

## Suggested Next

Step 3 only: validate selected `LirExtractValueOp` field/index/result
coherence using 801's native ordered field facts, with nearby positive and
malformed coverage.

## Watchouts

Do not repeat Steps 1--2; recover facts from display text; widen to other
aggregate/vector rows; reopen 801; weaken result/use/layout checks; or perform
Raw-BIR, CFG/PHI, lowering, MIR, or emission work.

## Proof

801 prerequisite proof accepted by the supervisor:
`cmake --build --preset default && ctest --test-dir build -j
--output-on-failure -R '^(frontend_hir_tests|frontend_lir_call_type_ref|frontend_lir_function_signature_type_ref)$'`
passed 3/3. It is prerequisite evidence only, not 754 Step 3 proof.
