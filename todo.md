# Current Packet

Status: Active
Source Idea Path: ideas/open/704_bir_semantic_handoff_views.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Add return and control-flow semantic views

## Just Finished

- Completed Step 4 by adding route-free `BirReturnView` and
  `BirControlFlowView` contracts for return identity/value/provenance and
  chain completeness, plus select dependencies, branch conditions, block
  relationships, and join sources.
- Added focused available, unavailable, incomplete, and ambiguous proof for
  both views, including public-header guards against route, prepared, ABI,
  destination, stack-load, and transfer authority vocabulary.

## Suggested Next

- Execute Step 5 as a bounded first common-MIR semantic-entry migration packet.

## Watchouts

- Return provenance is deliberately a source-value link and completeness fact;
  it does not grant any target return-location authority.
- Control-flow labels are exposed only as source relationships; the view does
  not prescribe executable transfers or publication placement.

## Proof

- Passed the supervisor-selected proof (308/308 backend tests):
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`.
- Focused return/control-flow behavior and source guard passed as part of the
  suite: `backend_bir_return_control_flow_view_contract`.
- Canonical proof log: `test_after.log`.
