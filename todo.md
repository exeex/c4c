# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.2
Current Step Title: Receive resolved direct integer-result calls

## Just Finished

- Plan Step 5.2: received resolved direct integer-result `LirCallOp` rows into
  Raw-BIR with owning source-value results, typed direct callee/signature/
  argument edges, immediate or same-function SSA arguments, verifier reachability,
  and transactional positive/negative receipt coverage.

## Suggested Next

- Select exactly one subsequent checked ordinary row for Plan Step 5.3.

## Watchouts

- Direct-call receipt remains restricted to resolved `LinkNameId`, integer result,
  fixed nonvariadic exact signatures, and immediate/current-function SSA arguments.
  Indirect, variadic, ABI-expanded, aggregate/object, unresolved, coercing, and
  floating forms remain unsupported without text recovery.

## Proof

- `cmake --build --preset default` followed by
  `ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`
  for Step 5.2; `test_after.log` is the preserved proof log.
