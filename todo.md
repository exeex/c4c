# Current Packet

Status: Active
Source Idea Path: ideas/open/742_lir_function_parameter_authority_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Verify exact relations and classify non-one-to-one shapes

## Just Finished

- Completed Plan Step 1 by invoking the existing
  `populate_lir_function_params` helper for ordinary definitions before FnCtx
  and body lowering, matching the declaration shell's structured HIR-owned
  publication path.
- Added direct producer coverage for empty and explicit-void parameter lists,
  plus unused fixed `int`/`long long`/`float`/`double` declaration-definition
  pairs with exact logical/signature/mirror count, order, and structured shape.
- Proved parameter names and retained signature rendering remain presentation:
  their drift does not replace the published TypeSpec and typed-mirror facts.

## Suggested Next

- Execute Plan Step 2 as a bounded verifier/classification packet: enforce only
  the proven plain one-to-one relationships while preserving truthful ABI-
  expanded distinctions.

## Watchouts

- `LirFunction.params` is now present on definitions but remains the logical
  list; it must not be flattened onto byval, HFA, vector, aggregate, narrow,
  pointer, function-pointer, or variadic ABI signature rows.
- Empty C `()` and explicit `(void)` remain distinct: empty has no logical
  parameter, while explicit void retains one logical sentinel and no fixed ABI
  parameter.
- The new-BIR receiver stayed unchanged and fail-closed throughout this packet.

## Proof

- Fresh `cmake --build --preset default` completed successfully.
- `ctest --test-dir build -R '^frontend_lir_function_signature_type_ref$'
  --output-on-failure` passed 1/1.
- `ctest --test-dir build -j --output-on-failure > test_after.log` passed
  3033/3033. The monotonic guard against `test_before.log` passed with delta
  `passed=0 failed=0` and no new over-30-second tests.
