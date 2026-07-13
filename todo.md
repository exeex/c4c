# Current Packet

Status: Active
Source Idea Path: ideas/open/734_lir_to_new_bir_container_completeness.md
Source Plan Path: plan.md
Current Step ID: 5.1
Current Step Title: Receive direct zero-argument void calls

## Just Finished

- Completed Plan Step 5.1's bounded modern `LirCallOp` receipt with a typed
  `CallNode` carrying only `FunctionId`, zero operands, and zero results.
- Added builder creation, immutable Raw/Canonical call views, foundation
  verification, and semantic dump rendering using only the target FunctionId.
- Reworked module import into create-all-functions then lower-definition-bodies
  passes, backed by a native `LinkNameId -> FunctionId` registry, so forward,
  recursive, and declaration/definition-merged calls are source-order safe.
- Kept `callee`, names, `args_str`, and `callee_type_suffix` presentation-only;
  all excluded result, argument, indirect, nonvoid, extended, variadic,
  unspecified, unresolved, and conflicting shapes reject atomically.
- Proved a production forward direct zero-argument void C call publishes
  `call fn1`; the producer supplied the exact structured row without blockers.

## Suggested Next

- Supervisor should validate this coherent Step 5.1 slice, commit it if
  accepted, and delegate lifecycle selection of the next bounded Step 5 row.

## Watchouts

- Call identity comes only from native `direct_callee_link_name_id`; `callee`,
  names, `args_str`, and `callee_type_suffix` are presentation only.
- Require a present structured void, zero-fixed, nonvariadic, specified callee
  signature that agrees with the resolved module function; do not parse text to
  fill missing or conflicting facts.
- Function creation must precede or otherwise safely support body target
  resolution so source order, forward calls, and valid recursion do not change
  semantics.
- Keep nonvoid/result calls, all arguments, indirect calls, variadic or
  unspecified signatures, CFG/local/body binding, and every other ordinary
  instruction row fail-closed.
- Preserve existing function linkage/elision merge rules and keep `long`/
  `unsigned long` blocked pending inactive idea 743.

## Proof

- Fresh `cmake --build --preset default` completed successfully.
- `ctest --test-dir build -R '^backend_lir_to_bir_interface$'
  --output-on-failure` passed 1/1 with builder, verifier, Raw/Canonical view,
  misleading-presentation, forward/recursive/merge, exclusion, and rollback
  coverage.
- `build/c4cll --dump-bir
  tests/backend/case/new_bir_direct_zero_arg_void_call.c` completed and printed
  `call fn1` from the production C row.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic guard against `test_before.log` passed with delta
  `passed=0 failed=0`, no new failures, and no new over-30-second tests.
