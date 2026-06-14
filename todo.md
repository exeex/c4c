Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 4
Current Step Title: Broader Backend Validation

# Current Packet

## Just Finished

Step 4 fixed the backend-validation regression in the byval pointer-source
classifier/consumer boundary.

The classifier now requires the prepared memory-access result name to agree
with the BIR load-local result while separately requiring the prepared
pointer-value address name to be a byval formal. This preserves the public MIR
consumer path where a load-local reads a value from a byval pointer without
requiring the loaded result itself to be the pointer base.

## Suggested Next

Hand this slice back to the supervisor for regression-log review, commit
boundary selection, and plan-owner retirement review for the byval
pointer-source classification candidate.

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
- `tests/backend/mir/backend_store_source_publication_plan_test.cpp` now
  exercises the recovered public positive path in the broader backend proof;
  no MIR fixture edits were needed.
- Residual risk is limited to the byval pointer-source classifier boundary:
  this slice does not address the direct-global select-chain dependency or
  source-value/source-producer metadata candidates.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`

Result: passed. The backend subset passed 180/180 after rebuilding the
publication planner and helper test. Output is preserved in `test_after.log`.
`git diff --check` also passed.
