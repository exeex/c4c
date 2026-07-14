# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Publish direct void scalar immediate argument authority

## Just Finished

- Completed Plan Step 3 for structured direct integer-call results.
- `emit_call_with_result` now allocates the claimed result with `fresh_value`,
  passes the native operand into the call maker, and returns the same operand
  through CallExpr lowering so the downstream return retains the exact ID.
- Added exact verifier coverage for required result authority and focused
  positive/malformed tests for same-ID reuse, misleading displays, missing
  result or structured return-ref authority, invalid, duplicate,
  unknown/cross-function IDs, and void-result authority.

## Suggested Next

- Execute Plan Step 4: publish direct void scalar immediate argument authority.

## Watchouts

- The common call-argument carrier must hold the native `LirOperand` and exact
  scalar type authority before any formatting.
- Step 4 owns only the immediate path for one fixed, nonvariadic, direct void
  call.
- `args_str` and type spelling are presentation only; do not parse them to
  create or repair argument authority.
- SSA argument reuse remains Step 5 and must consume the same common carrier,
  not an SSA-only side channel.
- Keep ABI, variadic, aggregate, indirect-call, CFG/terminator, stack/local/
  object, and body-parameter work outside Step 4.
- Preserve the accepted Step-3 scalar call-result path unchanged.

## Proof

- Fresh `cmake --build --preset default` passed for the Step-3 producer,
  carrier, verifier, and focused test changes.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof is in `test_after.log`.
- Supervisor acceptance guard generated a clean stashed full-suite baseline
  and after build. Matching `ctest --test-dir build -j --output-on-failure`
  runs passed 3033/3033 in both `test_before.log` and `test_after.log`.
- The monotonic regression delta was passed=0 and failed=0, with no new
  failures and no tests exceeding 30 seconds.
- `git diff --check` passed for the complete Step-3 slice.
