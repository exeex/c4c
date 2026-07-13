# Current Packet

Status: Active
Source Idea Path: ideas/open/744_lir_remaining_ordinary_value_identity_publication.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Extract and bind four focused one-contract probes (complete)

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

- Keep each focused probe one-contract as implementation proceeds; do not fold
  the four distinct first bad facts into a monolithic testcase or patch.
- Step 3 owns only scalar direct-call result publication and its verifier
  obligations; call-argument carrier work remains Steps 4-5.
- The SSA-argument implementation must reuse the argument carrier established
  by the immediate-argument seam; do not create a duplicate SSA-only carrier.
- Keep CFG/terminator targets, stack/local/alloca/object ownership, and body
  parameter identity outside Step 2.

## Proof

- Fresh `cmake --build --preset default` passed after adding the structural
  observations and four cases.
- `ctest --test-dir build -R '^frontend_lir_call_type_ref$' --output-on-failure > test_after.log`
  passed 1/1; canonical proof remains in `test_after.log`.
- `git diff --check` passed for the complete Step-2 slice.
