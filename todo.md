# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.31
Current Step Title: Publish direct scalar fp128 call-result authority on AArch64

## Just Finished

- Completed Plan Step 7.31: a direct, fixed, nonvariadic, zero-argument
  native `long double` call on AArch64 now allocates a fresh `LirValueId` for
  its actual `fp128` LIR return and preserves that exact ID into a same-type
  ordinary FAdd.
- The native-only floating verifier admits `fp128` alongside float, double,
  and x86_fp80 without source-spelling recovery. Focused AArch64 coverage
  rejects missing, invalid, duplicate, unknown, cross-owner, callee,
  signature, return, opcode, and binary-type authority conflicts while
  display-only mutations remain compatible.

## Suggested Next

- Select the next bounded packet from Plan Step 7 after supervisor review;
  retain the direct scalar call-result producer/verifier seam.

## Watchouts

- Step 7.31 excludes call arguments, indirect or variadic calls,
  ABI-expanded calls, conversions, other floating operations, BIR, and every
  separate family. Native IDs and type refs, never rendered spelling, govern
  the route; no new cross-operation type-provenance carrier is authorized.

## Proof

- Passed: `cmake --build --preset default && ctest --test-dir build -R
  '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log 2>&1`.
  The focused fp128 native-authority and display-independence checks ran in the
  call type-ref subset; proof log: `test_after.log`.
