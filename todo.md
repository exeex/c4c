Status: Active
Source Idea Path: ideas/open/204_route5_edge_join_source_adapter.md
Source Plan Path: plan.md
Current Step ID: 1
Current Step Title: Select The Route 5 Adapter Boundary

# Current Packet

## Just Finished

Step 1 selected exactly one adapter boundary:
`build_current_block_join_prepared_query_routing` in
`src/backend/mir/aarch64/codegen/dispatch_producers.cpp`.

The selected boundary may read Route 5 current-block join-source identity only
through `mir::find_bir_current_block_join_source_identity` and
`bir::Route5EdgeJoinSourceIndex`. Allowed Route 5 facts are successor block
identity, predecessor label/id for each PHI incoming, destination/source value
identity and type, named or immediate source kind, source producer kind,
source producer instruction/index, no-source or missing-publication status, and
current-block incoming expression/source identity.

Prepared state remains authoritative for block-entry move bundles, out-of-SSA
parallel-copy scheduling, source and destination homes, register/stack storage
policy, move order, scratch/cycle handling, branch policy, wrapper formatting,
final edge-copy records, coalescing, execution-site placement, and emitted
output. Route 5 must only decide whether an instruction result is a join
incoming expression or source candidate at this reader boundary; any invalid,
absent, conflicting, mismatched, or incomplete Route 5 identity falls back to
the existing prepared query path.

Targeted negative coverage for the next packet should include absent Route 5
index/data fallback, successor-label mismatch fallback, missing source
producer fallback, destination/source mismatch rejection, duplicate or
conflicting join-source record rejection, no-source status, and a memory-source
edge sanity case proving Route 5 memory facts do not become join scheduling or
storage authority.

## Suggested Next

Execute Step 2 for the selected
`build_current_block_join_prepared_query_routing` boundary: add or tighten the
fail-closed Route 5 current-block join-source adapter so only fully valid BIR
identity feeds the routing bitsets and every invalid case falls back to the
prepared query path.

## Watchouts

- Route 5 is semantic identity only; prepared state remains authoritative for
  scheduling, storage, wrappers, branch policy, and output.
- Do not broaden beyond one selected reader before the first proof ladder is
  complete.
- Do not claim progress through expected-output weakening, unsupported
  downgrades, helper renames, or testcase-shaped matching.

## Proof

Step 1 selection-only packet. No build or test run required, and
`test_after.log` was not produced.

Future Step 2 proof command:
`cmake --build --preset default && ctest --test-dir build -j --output-on-failure -R '^backend_'`.
