# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.30
Current Step Title: Publish direct scalar x86_fp80 call-result authority

## Just Finished

- Completed Plan Step 7.30: a direct, fixed, nonvariadic, zero-argument
  native `long double` call now allocates a fresh `LirValueId` when its actual
  default-target LIR return is `x86_fp80`, and preserves it into a same-type
  ordinary FAdd.
- The native-only floating verifier admits `x86_fp80` alongside float/double
  without source-spelling recovery. Focused x86_fp80 coverage
  rejects missing, invalid, duplicate, unknown, cross-owner, callee,
  signature, return, opcode, and binary-type authority conflicts while
  display-only mutations remain compatible.

## Suggested Next

- Supervisor: select the next bounded active-plan packet; this packet does not
  broaden direct call-result authority beyond the demonstrated native
  zero-argument x86_fp80 row.

## Watchouts

- The completed x86_fp80 row excludes call arguments, indirect, variadic, and
  ABI-expanded calls, conversions, other floating operations, BIR, and every
  separate family. Native IDs and type refs, never rendered spelling, govern
  the route; no new cross-operation type-provenance carrier was introduced.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`.
  The focused test log is `test_after.log`.
- Also passed: `git diff --check`.
