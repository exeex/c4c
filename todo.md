# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.29
Current Step Title: Publish direct scalar float call-result authority

## Just Finished

- Completed Plan Step 7.28: the existing direct-call producer now allocates a
  fresh `LirValueId` for exactly a direct, fixed, nonvariadic, zero-argument
  `double` result call, and its ordinary `double` FAdd use preserves that ID.
- Step 7.28's native-only verifier checks the module-owned direct callee ID,
  exact empty signature, matching `double` return refs, result authority, and
  floating use shape while display-only mutations remain compatible.

## Suggested Next

- Executor: implement only a direct, fixed, nonvariadic, zero-argument `float`
  result call whose exact fresh result ID flows to an ordinary `float` FAdd.
  Require the native direct-callee `LinkNameId`, zero-parameter structured
  signature, matching `float` return refs, a fresh result ID, same-ID FAdd use,
  display independence, and reachable rejection of missing, invalid, or
  duplicate results; unknown or cross-owner uses; callee/signature/count/
  variadic/return-type conflicts; and floating opcode/operand/type conflicts.

## Watchouts

- This packet excludes integer and `double` calls, call arguments, indirect,
  variadic, and ABI-expanded calls, conversions, other floating operations,
  BIR, and every separate family. Native IDs and type refs, never rendered
  spelling, govern the route.

## Proof

- Proposed proof: `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`.
- Also run `git diff --check`.
