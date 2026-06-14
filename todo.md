Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Nearby Fail-Closed Rows

# Current Packet

## Just Finished

Step 3 completed the narrowed nearby fail-closed proof rows for byval
pointer-source classification, following
`review/byval_pointer_source_review.md`.

Focused helper-test coverage now keeps the existing positive and Step 2
fail-closed rows intact and adds explicit rows for absent prepared addressing,
invalid prepared/BIR block-label identity, and non-byval formal fact distinct
from missing formal fact.

## Suggested Next

Execute Step 4 broader backend validation for this byval pointer-source
classification candidate.

## Watchouts

- Keep this packet limited to the byval pointer-source classification
  candidate.
- Do not absorb the direct-global select-chain dependency or
  source-value/source-producer metadata candidates.
- Do not reactivate completed module, names, control-flow, or recovered-source
  packets.
- Preserve prepared addressing authority, formal-name authority, public
  prepared aggregate compatibility, and the prepared pointer-name to BIR
  load-result agreement guard.
- Do not rewrite output expectations, diagnostics, helper statuses, or target
  output to claim progress.
- Leave `formal_publications.cpp`, `formal_publications.hpp`,
  `tests/backend/bir/backend_prepare_liveness_test.cpp`, target lowering,
  direct-global select-chain logic, recovered-source logic, source-producer
  metadata, printers, diagnostics, and output baselines out of Step 4 unless
  the supervisor delegates a wider packet.
- Existing MIR coverage in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp` exercises
  the current public positive path; Step 4 should use the supervisor-selected
  broader backend proof instead of adding new MIR or target rows unless
  explicitly delegated.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. The focused helper test rebuilt and
`backend_prepared_lookup_helper` passed 1/1. Output is preserved in
`test_after.log`. `git diff --check` also passed.
