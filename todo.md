# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 7.31
Current Step Title: Publish direct scalar fp128 call-result authority on AArch64

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

- Bounded packet 7.31: on the AArch64 target, preserve the native `fp128`
  result of one direct, fixed, nonvariadic, zero-argument scalar `long double`
  call into a same-type ordinary FAdd. Keep the existing direct scalar
  call-result producer/verifier seam; do not broaden the source contract.

## Watchouts

- Packet 7.31 excludes call arguments, indirect or variadic calls,
  ABI-expanded calls, conversions, other floating operations, BIR, and every
  separate family. Native IDs and type refs, never rendered spelling, govern
  the route; no new cross-operation type-provenance carrier is authorized.

## Proof

- Source boundary: the AArch64 direct, fixed, nonvariadic, zero-argument
  scalar `long double` call-result row only; its actual native LIR return must
  be `fp128` and its consumer a same-type ordinary FAdd.
- Required packet proof: a fresh build plus the narrow focused call type-ref
  coverage, including malformed native-authority and display-independence
  checks for the fp128 row; retain `git diff --check`.
