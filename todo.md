Status: Active
Source Idea Path: ideas/open/260_phase_f3_prepared_module_structural_one_reader_candidates.md
Source Plan Path: plan.md
Current Step ID: 2
Current Step Title: Implement the Prepared Agreement Boundary

# Current Packet

## Just Finished

Step 2 implemented the prepared agreement boundary for the direct-global
select-chain dependency lookup.

Completed work:
- Added a local `find_prepared_select_chain_source_producer_agreement(...)`
  helper in `select_chain_lookups.cpp` so the public producer lookup and the
  shared dependency classifier use the same complete prepared/BIR agreement
  record.
- Tightened `find_prepared_direct_global_select_chain_dependency(...)` so it
  publishes dependency facts only when the root producer agreement has a valid
  source producer, instruction pointer, same-block instruction index, and
  before-instruction cutoff.
- Added focused helper rows proving the positive prepared agreement path and
  fail-closed behavior for absent root producer, stale cutoff, conflicting
  block label, prepared/BIR value-name drift, incomplete root payload, invalid
  query block label, null block, and root-after-consumer cutoff.

## Suggested Next

Execute Step 3 by adding nearby fail-closed rows for the remaining direct-global
select-chain dependency edge cases without widening into the separate
source-value/source-producer metadata candidate.

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
- Step 3 should focus on nearby rows not already covered here: wrong producer
  kind, root not select when select-specific fields are expected, missing
  producer payload below the root, non-direct-global chains, producer after the
  consumer cutoff inside the chain, duplicate/conflicting producers, and public
  prepared compatibility assertions.
- Keep broad publication policy, storage encoding, pending store-global policy,
  duplicate handling, pointer-base homes, target lowering, call argument
  lowering, and prepared aggregate compatibility out of this runbook.

## Proof

Ran `cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_prepared_lookup_helper$'`.
Result: passed, `1/1` focused test passed.
Log: `test_after.log`.
