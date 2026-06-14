Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Add Nearby Fail-Closed Rows

# Current Packet

## Just Finished

Step 3 added nearby fail-closed rows for the direct-global select-chain
dependency lookup.

Completed work:
- Extended `verify_direct_global_select_chain_dependency_query()` with nearby
  rows for wrong producer kind, missing child producer payload, conflicted
  producer-map entries, non-direct-global select chains, and a child producer
  after the root cutoff.
- Added a public prepared compatibility assertion for the store-source
  direct-global select-chain dependency alias on a non-select direct-global
  root.
- Kept the Step 3 slice to helper-test coverage only; no implementation
  widening was needed.

## Suggested Next

Execute Step 4 by running the supervisor-selected broader backend validation
for this one-candidate runbook and record whether the direct-global
select-chain dependency packet is ready for plan-owner retirement review.

## Watchouts

- Keep this packet limited to the direct-global select-chain dependency
  candidate.
- Do not absorb the source-value/source-producer metadata candidate.
- Do not reactivate completed module, names, control-flow, recovered-source,
  or byval pointer-source packets.
- Preserve prepared source-producer authority, block identity checks, public
  prepared aggregate compatibility, and current fail-closed behavior.
- Do not rewrite output expectations, diagnostics, helper statuses, baselines,
  or target output to claim progress.
- Remaining validation should be proof-only unless broader backend validation
  exposes a local direct-global select-chain dependency failure.
- Keep broad publication policy, storage encoding, pending store-global policy,
  duplicate handling, pointer-base homes, target lowering, call argument
  lowering, and prepared aggregate compatibility out of this runbook.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.
Result: passed, `1/1` focused test passed.
Log: `test_after.log`.
