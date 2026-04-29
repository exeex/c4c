Status: Active
Source Idea Path: ideas/open/132_parser_rendered_record_template_lookup_mirror_cleanup.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Reproduce And Localize Signature Type-Ref Mismatch
你該做baseline sanity check了

# Current Packet

## Just Finished

Lifecycle correction reopened idea 132 after close commit `0e39decb` failed
baseline review on a rebuilt `frontend_lir_function_signature_type_ref` test.

## Suggested Next

Execute Step 1. Reproduce the failure, then trace `declared_pair` return type
identity through parser/HIR/LIR until the structured return type-ref mirror
first diverges from the expected `%struct.Big` identity.

Suggested proof command:
`ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$' --output-on-failure`

## Watchouts

Do not accept the pending baseline review while this test fails. Do not weaken
the LIR verifier, downgrade the test, or special-case `declared_pair`; repair
the structured identity or mirror-authority path that can produce the mismatch.

## Proof

Supervisor reported two direct reruns of
`ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$' --output-on-failure`
failed with:

`LirFunction.signature_return_type_ref: return mirror for function 'declared_pair' names a different structured return type than %struct.Big`
