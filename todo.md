Status: Active
Source Idea Path: ideas/open/02_aarch64_calls_emission_consolidation.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select One Retained Metadata Authority Leak

# Current Packet

## Just Finished

Step 5 closure review rejected closing the source idea after the outgoing stack
extent authority-removal slice.

- `test_before.log` records the accepted broad backend checkpoint command
  passing `162/162`.
- `calls_common.cpp` no longer has retained `CallInst::arg_abi` or
  `CallInst::arg_types` decision reads for outgoing stack extent.
- Closure remains blocked because publication and byval helper paths still
  read retained `CallInst::arg_abi` / `CallInst::arg_types` for call-boundary
  decisions that must be owned by prepared call facts.

## Suggested Next

Route execution back to Step 1 of the updated publication/byval checkpoint.
Select one surviving retained metadata authority leak:

- `calls_dispatch_bridge.cpp` local aggregate address publication eligibility.
- `calls_argument_sources.cpp` local frame address publication pointer/byval
  checks.
- `calls_byval_aggregates.cpp` byval size and indirect-register predicates.

## Watchouts

Do not reopen outgoing stack extent unless new evidence appears; the closure
review found no surviving `calls_common.cpp` retained ABI/type decision read.
Do not touch `ideas/open/03_dispatch_responsibility_reduction.md`.

## Proof

No new build was required for this lifecycle-only closure review.

Accepted broad proof baseline: `test_before.log` records
`(cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_')`
passing `162/162` backend tests with `100% tests passed, 0 tests failed out of
162`.
