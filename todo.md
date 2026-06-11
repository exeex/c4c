Status: Active
Source Idea Path: ideas/open/204_route5_edge_join_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 3
Current Step Title: Wire The Selected Reader To The Adapter

# Current Packet

## Just Finished

Step 3 verified `build_current_block_join_prepared_query_routing` already
consumes the validated Route 5 current-block join-source adapter and needs no
additional implementation wiring.

The selected reader builds a Route 5 edge/join-source index for the current
BIR function, passes it to `mir::find_bir_current_block_join_source_identity`,
and uses Route 5 only when that fail-closed adapter reports `Available`. In
that case, Route 5 contributes only incoming-expression and source semantic
identity bitsets for the current BIR block instructions. If the adapter is
absent, mismatched, duplicate, incomplete, no-source, missing-producer, or
otherwise unavailable, the boundary falls through to the existing prepared
query path.

No code changes were needed. Prepared state remains authoritative for move
scheduling, storage homes, move bundles, branch policy, wrappers, final
edge-copy records, coalescing, execution-site placement, and emitted output.

## Suggested Next

Execute Step 4: prove adapter completeness and route quality for the selected
boundary, using the existing Step 2/Step 3 proof as the starting point and
adding only focused adjacent proof if the supervisor wants more confidence
before acceptance.

## Watchouts

- Route 5 is semantic identity only; prepared state remains authoritative for
  scheduling, storage, wrappers, branch policy, and output.
- The indexed Route 5 adapter deliberately rejects duplicate exact
  join-source records and wrong-predecessor records even when destination and
  source values otherwise match.
- Memory-like named sources can identify join-source expressions, but they do
  not become scheduling or storage authority.
- Step 3 was intentionally a no-code verification slice; avoid adding churn
  unless Step 4 review identifies a real proof or route-quality gap.
- Do not claim progress through expected-output weakening, unsupported
  downgrades, helper renames, or testcase-shaped matching.

## Proof

Ran exactly:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_' > test_after.log`.

Result: passed. Proof log path: `test_after.log`.
