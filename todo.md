# Current Packet

Status: Active
Source Idea Path: ideas/open/716_prealloc_current_block_routing_authority_closure.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Establish the authority-collision baseline

## Just Finished

- Step 1 established the authority-collision baseline and removed the rejected
  Route 5-to-`Available` synthesis/replacement and fixture-policy masking
  changes from the uncommitted slice.
- Retained only the independent owner-attachment candidate seam:
  `make_function_lowering_context` owns `PreparedFunctionLookups` through a
  shared owner copied with `FunctionLoweringContext`; routing policy and query
  behavior are unchanged.
- Added and registered three one-primary-seam baseline probes for copied-owner
  lifetime, Route 5 diagnostic variation with no edge authority, and the four
  independent fixture policy/attachment combinations.
- Contract map: owner attachment/lifetime is registered as
  `backend_prealloc_current_block_lookup_attachment_lifetime`; Route 5
  diagnostic non-authority is registered as
  `backend_prealloc_route5_diagnostic_non_authority`; complete
  incoming-expression authority maps to registered
  `backend_prepared_fact_boundary_contract`; fixture policy versus attachment
  is registered as `backend_aarch64_current_block_fixture_policy_attachment`;
  bounded integration maps to registered
  `backend_aarch64_current_block_join_routing`.

## Suggested Next

- Execute Step 2 by strengthening the registered
  `backend_prealloc_current_block_lookup_attachment_lifetime` baseline to
  prove construction occurs at the real function-context owner, copied
  contexts retain lookup lifetime, and missing attachment fails closed,
  without asserting any routing answer.

## Watchouts

- Ideas 713 and 705 remain open and blocked pending handback.
- Rejected route 1 first lacked complete prepared edge authority at the query
  boundary: owner attachment plus target/BIR fallback could answer without an
  invariant predecessor/destination/semantic-origin fact set.
- Rejected route 2 first lacked an authoritative incoming-expression fact
  independent of Route 5: a unique Route 5 diagnostic identity replaced the
  prepared incoming-expression set and was then promoted to `Available`.
- The three new probes are fail-closed baselines, not later-step completion:
  Step 2 must bind construction to the real function owner, Step 3 must hold a
  real authoritative edge fact set constant while Route 5 diagnostics vary,
  and Step 5 must connect its four fixture axes to real policy construction
  without changing existing integration vectors.

## Proof

- Passed `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' 2>&1 | tee test_after.log`.
- Result: 316/316 backend tests passed; proof log: `test_after.log`.
