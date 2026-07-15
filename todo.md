# Current Packet

Status: Step 2 complete
Source Idea Path: ideas/open/761_lir_call_signature_type_mirror_convergence.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Converge the selected typed call/signature path

## Just Finished

- Step 2: made complete fixed call signatures and their matching structured
  argument/type-reference mirrors authoritative in the LIR verifier. This path
  now validates native `LirTypeRef` agreement without consulting `args_str`,
  the callee suffix, `LirCallArg::type`, or `fixed_param_types`; incomplete
  mirrors retain the raw compatibility validation path. Added backend coverage
  that verifies structured `i32 %actual` facts despite stale `i64` text and
  still rejects a real structured fixed-signature mismatch.

## Suggested Next

- Select the next bounded runbook packet; do not widen this completed verifier
  packet into printing, lowering, selector, or inline-assembly work.

## Watchouts

- Empty, partial, variadic, or unspecified structured argument mirrors remain
  on the explicit raw compatibility path because they cannot prove complete
  fixed-call type authority.

## Proof

- `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' | tee test_after.log` — passed (5/5 backend tests); proof log: `test_after.log`.
