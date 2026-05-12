Status: Active
Source Idea Path: ideas/open/186_bir_direct_symbol_identity_validation_closure.md
Source Plan Path: plan.md
Current Step ID: Step 3
Current Step Title: Add focused direct-symbol tests

# Current Packet

## Just Finished

Step 3 completed for idea 186. Focused direct-symbol coverage was reviewed and
no additional tests were needed in this packet.

The existing focused backend tests account for:

- structured direct-call success with `direct_callee_link_name_id` selecting the
  declared semantic callee despite drifted display spelling
- structured direct-call missing `direct_callee_link_name_id` rejection
- structured direct-call present-but-unknown/stale `LinkNameId` rejection
- direct-call verifier rejection for known id/display-name mismatches, unknown
  ids, and id-only references to undeclared functions
- pointer-initializer present `initializer_function_link_name_ids` fail-closed
  behavior instead of raw function-symbol fallback
- pointer-initializer and direct-call raw/no-id compatibility paths
- runtime/intrinsic placeholder coverage remaining in explicit runtime
  families rather than the metadata-rich direct-call route

## Suggested Next

Run Step 4 validation and handoff for idea 186: preserve the backend proof log,
record final residual risks, and recommend supervisor lifecycle close/switch
review if the proof remains green.

## Watchouts

- The direct-call fence keys on structured `callee_signature`; runtime and
  placeholder compatibility should remain on no-signature/raw call routes.
- Raw/no-id direct calls without structured callee metadata remain accepted.
- Pointer initializer `initializer_function_link_name_ids` already fails
  closed on present-but-unknown ids and should not be widened through raw
  lookup.
- Step 3 did not edit test source because the required coverage already exists
  across focused lowering tests and BIR verifier tests.

## Proof

`git diff --check -- todo.md` passed.

Proof command passed:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log 2>&1`

`test_after.log`: 100% tests passed, 0 failed out of 109 backend tests.
