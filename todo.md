Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the Prepared Agreement Boundary

# Current Packet

## Just Finished

Step 2 implemented the prepared agreement boundary for byval pointer-source
classification. `prepared_store_source_load_local_is_byval_formal_pointer_source(...)`
now accepts only when the prepared load-local source producer, the prepared
memory-access pointer-value addressing row, the prepared pointer value name,
`can_use_base_plus_offset`, and the BIR byval formal-name fact all agree.

Focused helper-test coverage now exercises the public
`populate_store_source_publication_plans(...)` consumer path for positive
agreement plus fail-closed rows for absent source producer authority, wrong
source-producer kind, missing prepared load access, non-pointer prepared
addressing, missing pointer value name, disallowed base-plus-offset policy,
prepared pointer-name/BIR load-result mismatch, and missing byval formal fact.

## Suggested Next

Execute Step 3 by adding or confirming any remaining nearby public consumer
rows for byval pointer-source classification. Keep the next packet local to
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp` and
`todo.md` unless review finds a concrete local gap in
`src/backend/prealloc/publication_plans.cpp`.

## Watchouts

- Keep this packet limited to the byval pointer-source classification
  candidate.
- Do not absorb the direct-global select-chain dependency or
  source-value/source-producer metadata candidates.
- Do not reactivate completed module, names, control-flow, or recovered-source
  packets.
- Preserve prepared addressing authority, formal-name authority, public
  prepared aggregate compatibility, and the newly added prepared pointer-name
  to BIR load-result agreement guard.
- Do not rewrite output expectations, diagnostics, helper statuses, or target
  output to claim progress.
- Leave `formal_publications.cpp`, `formal_publications.hpp`,
  `tests/backend/bir/backend_prepare_liveness_test.cpp`, target lowering,
  direct-global select-chain logic, recovered-source logic, source-producer
  metadata, printers, diagnostics, and output baselines out of Step 2 unless
  the supervisor delegates a wider packet.
- Existing MIR coverage in
  `tests/backend/mir/backend_store_source_publication_plan_test.cpp` exercises
  the current public positive path, but the next focused packet should avoid
  moving into MIR or target coverage unless the supervisor widens ownership.

## Proof

Ran delegated proof:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. The focused helper test rebuilt and
`backend_prepared_lookup_helper` passed 1/1. Output is preserved in
`test_after.log`. `git diff --check` also passed.
