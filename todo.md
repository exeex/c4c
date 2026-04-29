Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Baseline Review And Reclose Readiness
你該做test baseline review了

你該做baseline sanity check了

# Current Packet

## Just Finished

Steps 1 and 2 are satisfied by implementation commit `2c722228 Own LIR function
signature type tags`.

Step 1 localized the first bad fact to LIR storage: `LirFunction.return_type`
and signature parameter TypeSpecs retained raw frontend `TypeSpec::tag`
pointers, so later copied-module string mutations could make
`declared_pair.return_type` identify as `%struct.Big` while its signature mirror
still carried `%struct.Pair`.

Step 2 repaired that authority path. LIR lowering now stores LIR-owned tag
strings for function return/parameter TypeSpecs, and aggregate type-ref
construction derives `StructNameId` from the structured tag rather than from
rendered mirror text. Reviewer artifact `review/reviewA.md` found no blocking
route or overfit issue and recommended continuing to Step 3.

## Suggested Next

Run the supervisor-selected broader baseline or regression guard for Step 3,
then request close review only if the fresh baseline proof is sufficient for
reclose readiness.

## Watchouts

No verifier weakening or named fixture special-casing was used. The current
repair covers function return types, function params, and structured signature
params. Reviewer artifact `review/reviewA.md` treats globals or stack objects
with retained TypeSpec tags as a watch item rather than a blocker for this
repair.

Broader proof is still required before renewed close readiness because the
implementation touched shared LIR aggregate type-ref construction and LIR module
metadata ownership.

## Proof

Built the focused test target, then ran:

`ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$' --output-on-failure`

Result: passed, 1/1 tests. Proof log: `test_after.log`.

Original failure reproduced before the repair with:

`ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$' --output-on-failure`

`LirFunction.signature_return_type_ref: return mirror for function 'declared_pair' names a different structured return type than %struct.Big`

Supervisor also reported a five-test frontend/LIR sanity subset passed. Step 3
still needs the selected broader baseline or regression-guard proof before
reclose readiness.
