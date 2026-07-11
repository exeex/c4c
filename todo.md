# Current Packet

Status: Active
Source Idea Path: ideas/open/704_bir_semantic_handoff_views.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add call and comparison semantic views

## Just Finished

- Completed Step 3 by adding route-free `BirCallBoundaryView` and
  `BirComparisonView` contracts. The named results expose only semantic
  call/callee/argument/dependency/result and
  comparison/operand/predicate/materialized-condition/branch-use facts.
- Added focused behavior proof for available, unavailable, incomplete, and
  ambiguous outcomes for both views, plus source guards rejecting route,
  prepared, ABI, and control-transfer authority vocabulary in the public
  headers.

## Suggested Next

- Execute Step 4 as a bounded return/control-value semantic-view packet,
  preserving source semantics without prepared placement or executable
  control authority.

## Watchouts

- The former public `BirCallBoundaryView` wrapper in `bir.hpp` is now a
  `Route6CallBoundaryAdapter`; the compatibility function overload taking a
  `Function` remains for existing consumers, while the named header owns the
  new block-level semantic overload.
- The named comparison result reports only whether its materialized condition
  is used by the block branch; it intentionally does not expose branch labels.

## Proof

- Passed the supervisor-selected proof (307/307 backend tests):
  `set -o pipefail; (cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_') 2>&1 | tee test_after.log`.
- Focused contract behavior/source guard passed:
  `ctest --test-dir build --output-on-failure -R '^backend_bir_call_comparison_view_contract$'`.
- Canonical proof log: `test_after.log`.
