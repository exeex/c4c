Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the Prepared/BIR Agreement Boundary

# Current Packet

## Just Finished

Step 2 implemented the prepared/BIR agreement boundary for recovered
narrow-store source lookup.

- Added a local agreement gate in
  `find_prepared_recovered_narrow_store_source_for_wide_local_load(...)` that
  accepts the prepared recovered source only when Route 3 BIR identity agrees
  on the load/store instructions, local-slot memory facts, byte offsets, access
  sizes, and stored value identity.
- Preserved prepared addressing authority: missing prepared load/store access,
  missing frame-slot IDs, wrong widths, wrong lanes, or missing prepared scan
  evidence still return `std::nullopt`.
- Added focused helper-test coverage for positive recovered-source agreement,
  missing BIR memory identity, and prepared/BIR mismatch.

## Suggested Next

Execute Step 3 by adding nearby fail-closed rows around the recovered-source
boundary if the supervisor wants explicit coverage beyond the Step 2 rows.
Expected owned file is
`tests/backend/bir/backend_prepared_lookup_helper_test.cpp`, plus `todo.md`.

## Watchouts

- Keep this packet limited to the recovered-source identity candidate.
- Do not absorb the byval pointer-source, direct-global select-chain, or
  source-value/source-producer metadata candidates.
- The Route 3 stored-value identity only confirms same local-slot range
  evidence; the prepared recovered-source helper remains the policy owner for
  the narrow-store/wide-load scan and width checks.
- Preserve prepared publication planner policy, source-producer lookup
  authority at the caller, public prepared aggregate compatibility, and current
  fail-closed behavior.
- Do not rewrite output expectations, diagnostics, helper statuses, or target
  output to claim progress.
- Surfaces explicitly out of scope for this runbook packet: byval formal pointer-source
  classification, direct-global select-chain dependency lookup, source-value
  and source-producer metadata packets, target output/diagnostics/baselines,
  store publication status semantics, frame-slot policy, pointer-base homes,
  pending publication policy, and duplicate-publication handling.

## Proof

Ran:

`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`

Result: passed. The focused helper test rebuilt and
`backend_prepared_lookup_helper` passed 1/1. `git diff --check` also passed.

Proof log: `test_after.log`.
