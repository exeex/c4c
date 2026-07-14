# Current Packet

Status: Complete
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 7.13
Current Step Title: Receive the checked wide builtin-ffs select narrowing

## Just Finished

- Step 7.13 complete: received only the producer-verified i64 wide
  `LirSelectOp` result through an operand-free typed Raw-BIR select receipt,
  then its exact i64-to-i32 `LirCastOp::Trunc` and later ordinary i32 `Add`.
  The builder, view, verifier, source-ID registry, importer, and focused test
  preserve native type/result/edge authority and reject missing, invalid,
  duplicate, unresolved, wrong-kind/direction/endpoint, and missing-use forms
  transactionally without receiving the shared ffs arms or condition.

## Suggested Next

- Ask the plan owner to determine the next receiver-ready source-matrix row;
  Step 7.13 exhausted its bounded select/trunc receipt scope.

## Watchouts

- The Raw-BIR select node deliberately has no operands: ffs condition and arms
  remain producer-verified LIR authority and must not become an implicit
  general Select receipt route.

## Proof

- Passed 2/2: `cmake --build --preset default && ctest --test-dir build -j
  --output-on-failure -R
  '^backend_lir_to_bir_interface$|^frontend_lir_call_type_ref$'`. Per the
  packet's ownership constraint, canonical `test_after.log` was not modified.
