Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize Signature Type-Ref Mismatch
你該做test baseline review了

你該做baseline sanity check了

# Current Packet

## Just Finished

Step 1 localized and repaired the signature type-ref mismatch in the same
packet. The first bad fact was in LIR storage: `LirFunction.return_type` and
signature parameter TypeSpecs retained raw frontend `TypeSpec::tag` pointers,
so later copied-module string mutations could make `declared_pair.return_type`
identify as `%struct.Big` while its signature mirror still carried `%struct.Pair`.
LIR lowering now stores LIR-owned tag strings for function return/parameter
TypeSpecs, and aggregate type-ref construction derives `StructNameId` from the
structured tag rather than from rendered mirror text.

## Suggested Next

Supervisor should review whether Step 2 can be treated as satisfied by this
repair or whether a separate narrow packet should stabilize any remaining
LIR-held TypeSpec tag fields outside function signature metadata.

## Watchouts

No verifier weakening or named fixture special-casing was used. The current
repair covers function return types, function params, and structured signature
params; globals or stack objects with retained TypeSpec tags were not broadened
in this packet.

## Proof

Built the focused test target, then ran:

`ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$' --output-on-failure`

Result: passed, 1/1 tests. Proof log: `test_after.log`.

Original failure reproduced before the repair with:

`ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$' --output-on-failure`

`LirFunction.signature_return_type_ref: return mirror for function 'declared_pair' names a different structured return type than %struct.Big`
