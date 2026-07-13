# Current Packet

Status: Active
Source Idea Path: ideas/open/742_lir_function_parameter_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify exact relations and classify non-one-to-one shapes

## Just Finished

- Added reachable LIR verification for the proven fixed nonvariadic plain-
  scalar relationship: logical parameters, structured ABI signature rows, and
  typed mirrors must agree exactly in count, order, `TypeSpec`, kind, and width.
- Added structured mutation coverage for missing logical/signature/mirror
  rows, reorder, scalar-type conflict, logical-shape conflict, and mirror-width
  conflict; every malformed module rejects through `verify_module`.
- Preserved presentation-only names/signature text and explicitly exempted
  narrow, pointer, byval, aggregate/HFA/vector, complex, function-pointer,
  va-list, and variadic shapes from forced scalar parity.

## Suggested Next

- Finish Plan Step 2 with the bounded authority/disposition classification for
  every non-one-to-one row, without adding receiver implementation or forcing
  logical/ABI parity.

## Watchouts

- The scalar verifier intentionally activates when either the logical track is
  wholly proven plain or the signature track is wholly plain without an ABI-
  expanded logical carrier; this makes missing tracks reject without pulling
  aggregate/vector/complex expansion into the scalar contract.
- `LirFunction.params` remains the logical list and must not be flattened onto
  byval, HFA, vector, aggregate, narrow, pointer, function-pointer, va-list, or
  variadic ABI signature rows.
- Empty C `()` and explicit `(void)` remain distinct: empty has no logical
  parameter, while explicit void retains one logical sentinel and no fixed ABI
  parameter.
- The new-BIR receiver stayed unchanged and fail-closed throughout this packet.

## Proof

- Fresh `cmake --build --preset default` completed successfully after the final
  verifier audit.
- `ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$'
  --output-on-failure` passed 1/1.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic guard against `test_before.log` passed with delta
  `passed=0 failed=0` and no new over-30-second tests.
