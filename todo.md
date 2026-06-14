Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Nearby Fail-Closed Rows

# Current Packet

## Just Finished

Step 3 added nearby fail-closed proof rows for the recovered narrow-store
source identity boundary.

- Kept the existing positive prepared/BIR agreement path observable.
- Added direct helper rows proving `std::nullopt` for missing addressing,
  missing prepared load/store access rows, missing BIR load/store memory
  access, prepared/BIR frame-slot mismatch, lane offset mismatch, non-byte
  store width, wrong stored value, after-store ordering, missing BIR identity,
  and prepared/BIR byte-offset mismatch.
- Confirmed source-producer kind is not directly reachable in
  `find_prepared_recovered_narrow_store_source_for_wide_local_load(...)`; the
  recovered-source helper receives the already-selected load-local producer
  from its caller.

## Suggested Next

Execute Step 4 broader backend validation for the recovered-source identity
candidate.

## Watchouts

- Keep this packet limited to the recovered-source identity candidate.
- Do not absorb the byval pointer-source, direct-global select-chain, or
  source-value/source-producer metadata candidates.
- The Route 3 stored-value identity only confirms same local-slot range
  evidence; the prepared recovered-source helper remains the policy owner for
  the narrow-store/wide-load scan and width checks.
- Missing source-producer and wrong source-producer-kind rows belong at the
  caller/source-producer selection surface, not the direct recovered-source
  helper signature covered by this packet.
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
