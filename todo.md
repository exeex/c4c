# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.29
Current Step Title: Publish direct scalar float call-result authority

## Just Finished

- Completed Plan Step 7.29: a direct, fixed, nonvariadic, zero-argument
  native `float` call now allocates a fresh `LirValueId` and preserves it into
  an ordinary `float` FAdd, alongside the existing direct `double` route.
- The native-only float verifier requires module-owned direct callee ID, exact
  empty signature, matching `float` return refs, and result authority; focused
  coverage rejects result, ownership, signature, return, opcode, and operand
  type conflicts while display-only mutations remain compatible.

## Suggested Next

- Supervisor: select the next bounded active-plan packet; this packet does not
  broaden direct call-result authority beyond the completed zero-argument
  `float` row.

## Watchouts

- The completed float row excludes call arguments, indirect, variadic, and
  ABI-expanded calls, conversions, other floating operations, BIR, and every
  separate family. Native IDs and type refs, never rendered spelling, govern
  the route.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`.
  The focused test log is `test_after.log`.
- Also passed: `git diff --check`.
