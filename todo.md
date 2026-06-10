Status: Active
Source Idea Path: ideas/open/163_bir_edge_join_source_annotation_schema.md
Source Plan Path: plan.md
Current Step ID: 5
Current Step Title: Broaden Validation And Prepare Closure

# Current Packet

## Just Finished

Completed Step 5 for
`ideas/open/163_bir_edge_join_source_annotation_schema.md`.
Ran broad backend acceptance validation and prepared closure readiness for the
Route 5 edge/join-source schema migration:

- Route 5 BIR edge and current-block join-source records and indexes are
  BIR-owned semantic schema.
- `mir::find_bir_cfg_edge_publication_source_identity(...)` is migrated to
  Route 5 BIR records/index helpers.
- `mir::find_bir_current_block_join_source_identity(...)` remains
  intentionally unswitched, with Route 5 records/indexes implemented and
  oracle-covered.
- Prepared edge/join reads remain comparison oracles.
- Target/prealloc parallel-copy scheduling, move emission, storage, register
  policy, and target/codegen consumers were not imported or rewritten.

Closure readiness: broad backend validation matched the before-log scale with
`179/179` backend tests passing, and no expectation downgrades,
parallel-copy policy leakage, whole prepared record copies, target/codegen
rewrites, or helper-only reshuffles were accepted.

## Suggested Next

Supervisor/plan-owner can review closure readiness for idea 163 against the
active source idea and decide whether to close or carry a follow-up packet.

## Watchouts

- Residual implementation boundary: current-block join-source MIR query was
  intentionally not migrated in this plan step; its Route 5 records/indexes
  are present and oracle-covered.
- Route 5 schema remains semantic and target-neutral; prepared/target policy
  continues to own parallel-copy ordering, move bundles, execution placement,
  destination homes, storage encodings, register views, and emission decisions.
- `test_before.log` was already a matching broad green baseline at `179/179`;
  `test_after.log` now contains the matching broad after proof.

## Proof

Exact delegated Step 5 proof passed:

```bash
cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log
```

Additional local validation: `git diff --check` passed.

Proof log: `test_after.log` (`179/179` matching backend tests passed).
