# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Publish direct scalar call result identity

## Just Finished

- Completed Plan Step 2 by extracting four minimal cases for the scalar call
  result, immediate call argument, selected-global-load SSA call argument, and
  two-operation ordinary scalar result/use chain.
- Added structural frontend observations for each production boundary. The
  observations inspect instruction variants, native IDs, opcode/type refs, and
  monostate authority directly; they do not parse rendered LIR or compare
  display spelling to establish identity.
- Bound every probe in the authority matrix to its exact producer/carrier
  transition, post-publication verifier rejection obligations, and forbidden
  text fallback. The SSA argument remains explicitly dependent on the common
  carrier introduced for immediate arguments, not an SSA-only side carrier.

## Suggested Next

- Execute Plan Step 3: publish direct scalar call result identity.

## Watchouts

- Allocate the result through the current function's `LirValueId` authority
  before rendering.
- Preserve the same `LirOperand` into downstream scalar expression and return
  use; do not stop authority publication at the call node.
- Void call results must remain empty and carry no result authority.
- Keep call arguments, indirect calls, ABI work, CFG/terminator targets,
  stack/local/object ownership, and body-parameter identity outside Step 3.
- Do not add a text-to-ID map or recover result identity from display spelling.

## Proof

- Fresh `cmake --build --preset default` passed after adding the structural
  observations and four cases.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof remains in `test_after.log`.
- `git diff --check` passed for the complete Step-2 slice.
